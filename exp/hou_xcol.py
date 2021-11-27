# Author: Sergey Chaban <sergey.chaban@gmail.com>

import sys
import hou
import os
import imp
import re
import struct
import inspect
from math import *

import xcore
import xhou

try: xrange
except: xrange = range

class ColBVHNode:
	def __init__(self, bvh):
		self.bvh = bvh
		self.ipols = []
		self.left = None
		self.right = None
		self.bvh.addNode(self)

	def addPols(self, idx, cnt):
		for i in xrange(cnt):
			self.ipols.append(self.bvh.ipols[idx + i])
		self.bboxFromPols(idx, cnt)

	def bboxFromPols(self, idx, cnt):
		pol = self.bvh.col.pols[self.bvh.ipols[idx]]
		self.bboxMin = [pol.bboxMin[i] for i in xrange(3)]
		self.bboxMax = [pol.bboxMax[i] for i in xrange(3)]
		for i in xrange(1, cnt):
			pol = self.bvh.col.pols[self.bvh.ipols[idx + i]]
			for j in xrange(3):
				self.bboxMin[j] = min(self.bboxMin[j], pol.bboxMin[j])
				self.bboxMax[j] = max(self.bboxMax[j], pol.bboxMax[j])

	def build(self, idx, cnt, axis):
		if cnt == 1:
			self.addPols(idx, cnt)
		elif cnt == 2:
			self.left = ColBVHNode(self.bvh)
			self.left.addPols(idx, 1)
			self.right = ColBVHNode(self.bvh)
			self.right.addPols(idx+1, 1)
			self.bboxMin = [min(self.left.bboxMin[i], self.right.bboxMin[i]) for i in xrange(3)]
			self.bboxMax = [max(self.left.bboxMax[i], self.right.bboxMax[i]) for i in xrange(3)]
		else:
			self.bboxFromPols(idx, cnt)
			pivot = (self.bboxMin[axis] + self.bboxMax[axis]) * 0.5
			mid = self.bvh.split(idx, cnt, pivot, axis)
			nextAxis = (axis + 1) % 3
			self.left = ColBVHNode(self.bvh)
			self.right = ColBVHNode(self.bvh)
			self.left.build(idx, mid, nextAxis)
			self.right.build(idx + mid, cnt - mid, nextAxis)

	def writeInfo(self, bw):
		ipol = -1
		ileft = -1
		iright = -1
		if len(self.ipols) > 0: ipol = self.ipols[0]
		if self.left: ileft = self.bvh.nodeMap[self.left]
		if self.right: iright = self.bvh.nodeMap[self.right]
		if iright < 0:
			bw.writeI32(ipol)
		else:
			bw.writeI32(ileft)
		bw.writeI32(iright)

class ColBVH:
	def __init__(self, col):
		self.col = col
		npol = len(self.col.pols)
		self.ipols = [i for i in xrange(npol)]
		self.nodeLst = []
		self.nodeMap = {}
		self.root = ColBVHNode(self)
		bbsize = [(self.col.bboxMax[i] - self.col.bboxMin[i]) for i in xrange(3)]
		bbr = max(bbsize)
		axis = 0
		if bbr == bbsize[1]: axis = 1
		elif bbr == bbsize[2]: axis = 2
		self.root.build(0, npol, axis)

	def addNode(self, node):
		self.nodeMap[node] = len(self.nodeLst)
		self.nodeLst.append(node)

	def split(self, idx, cnt, pivot, axis):
		mid = 0
		for i in xrange(cnt):
			cpol = self.col.pols[self.ipols[idx + i]]
			cent = (cpol.bboxMin[axis] + cpol.bboxMax[axis]) * 0.5
			if cent < pivot:
				t = self.ipols[idx + i]
				self.ipols[idx + i] = self.ipols[idx + mid]
				self.ipols[idx + mid] = t
				mid += 1
		if mid == 0 or mid == cnt: mid = cnt // 2
		return mid

	def writeNodeBoxes(self, bw):
		for node in self.nodeLst:
			bw.writeFV(node.bboxMin)
			bw.writeFV(node.bboxMax)

	def writeNodeInfos(self, bw):
		for node in self.nodeLst:
			node.writeInfo(bw)

class ColGrp:
	def __init__(self, col, path):
		self.col = col
		self.path = path
		self.pathId = -1
		self.nameId = -1
		self.name = self.path
		if path:
			sep = self.path.rfind("/")
			if sep >= 0:
				self.pathId = self.col.strLst.add(self.path[:sep])
				self.name = self.name[sep+1:]
		self.nameId = self.col.strLst.add(self.getName())

	def getName(self):
		if self.name: return self.name
		return "$default"

class ColExporter(xcore.BaseExporter, xhou.PolModel):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		xhou.PolModel.__init__(self)
		self.sig = "XCOL"

	def build(self, sop):
		self.bboxMin = [0.0 for i in xrange(3)]
		self.bboxMax = [0.0 for i in xrange(3)]
		xhou.PolModel.build(self, sop)
		self.setNameFromPath(self.sop.parent().path())
		npol = len(self.pols)
		if npol < 1:
			return
		xhou.PolModel.initMtls(self)
		self.grps = []
		for mtlPath in self.mtlPaths:
			self.grps.append(ColGrp(self, mtlPath))
		self.bvh = ColBVH(self)
		self.maxVtxPerPol = 0
		for pol in self.pols:
			self.maxVtxPerPol = max(self.maxVtxPerPol, pol.numVtx())
		self.allPolsSameSize = True
		nvtx = self.pols[0].numVtx()
		for i in xrange(1, npol):
			if nvtx != self.pols[i].numVtx():
				self.allPolsSameSize = False
				break

	def writeHead(self, bw, top):
		npnt = len(self.pnts)
		npol = len(self.pols)
		ngrp = len(self.grps)
		ntri = self.ntris

		bw.writeFV(self.bboxMin) # +20
		bw.writeFV(self.bboxMax) # +2C
		bw.writeU32(npnt) # +38
		bw.writeU32(npol) # +3C
		bw.writeU32(ngrp) # +40
		bw.writeU32(ntri) # +44
		bw.writeU32(self.maxVtxPerPol) # +48
		bw.writeU32(0) # +4C reserved
		self.patchPos = bw.getPos()
		bw.writeU32(0) # +50 -> pnts
		bw.writeU32(0) # +54 -> pol idx org
		bw.writeU32(0) # +58 -> pol nvtx
		bw.writeU32(0) # +5C -> pol tri org
		bw.writeU32(0) # +60 -> pol idx
		bw.writeU32(0) # +64 -> pol grps
		bw.writeU32(0) # +68 -> pol tri idx
		bw.writeU32(0) # +6C -> pol bboxes
		bw.writeU32(0) # +70 -> pol nrms
		bw.writeU32(0) # +74 -> grp info
		bw.writeU32(0) # +78 -> BVH bboxes
		bw.writeU32(0) # +7C -> BVH infos

	def writeData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 0, bw.getPos() - top) # -> pnts
		for pos in self.ppos: bw.writeFV(pos)

		bw.align(0x10)
		bw.patch(self.patchPos + 0x1C, bw.getPos() - top) # -> pol bboxes
		for pol in self.pols:
			bw.writeFV(pol.bboxMin)
			bw.writeFV(pol.bboxMax)

		if self.bvh:
			bw.align(0x10)
			bw.patch(self.patchPos + 0x28, bw.getPos() - top) # -> BVH bboxes
			self.bvh.writeNodeBoxes(bw)

		bw.align(4)
		bw.patch(self.patchPos + 0x20, bw.getPos() - top) # -> pol nrms
		for pol in self.pols:
			onrm = xcore.encodeOcta(pol.nrm)
			for i in xrange(2): onrm[i] = (onrm[i] + 1.0) * 0.5
			bw.writeU16(xcore.u16(onrm[0]))
			bw.writeU16(xcore.u16(onrm[1]))

		bw.align(4)
		bw.patch(self.patchPos + 4, bw.getPos() - top) # -> pol idx org
		idxOrg = 0
		for pol in self.pols:
			nvtx = pol.numVtx()
			bw.writeI32(idxOrg)
			idxOrg += nvtx

		if not self.maxVtxPerPol == 3:
			bw.align(4)
			bw.patch(self.patchPos + 0xC, bw.getPos() - top) # -> pol tri org
			triOrg = 0
			for pol in self.pols:
				nvtx = pol.numVtx()
				otri = -1
				if nvtx > 3:
					otri = triOrg
				bw.writeI32(otri)
				if nvtx > 3:
					triOrg += len(pol.tris)

		bw.align(4)
		bw.patch(self.patchPos + 0x10, bw.getPos() - top) # -> pol idx
		for pol in self.pols:
			for pid in pol.pids: bw.writeI32(pid)

		if self.bvh:
			bw.align(0x10)
			bw.patch(self.patchPos + 0x2C, bw.getPos() - top) # -> BVH infos
			self.bvh.writeNodeInfos(bw)

		bw.align(4)
		bw.patch(self.patchPos + 0x24, bw.getPos() - top) # -> grp info
		for grp in self.grps:
			self.writeStrId16(bw, grp.nameId)
			self.writeStrId16(bw, grp.pathId)

		if len(self.grps) > 1:
			bw.patch(self.patchPos + 0x14, bw.getPos() - top) # -> pol grps
			for pol in self.pols:
				bw.writeU8(pol.imtl)

		if not self.allPolsSameSize:
			bw.patch(self.patchPos + 8, bw.getPos() - top) # -> pol nvtx
			for pol in self.pols:
				bw.writeU8(pol.numVtx())

		if not self.maxVtxPerPol == 3:
			bw.patch(self.patchPos + 0x18, bw.getPos() - top) # -> pol tri idx
			for pol in self.pols:
				if pol.numVtx() > 3:
					for irel in pol.tris:
						bw.writeU8(irel)


	def save(self, outPath):
		if self.allPolsSameSize: self.flags |= 1
		xcore.BaseExporter.save(self, outPath)
