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

class MotTrk:
	def __init__(self, data):
		self.mask = 0
		self.bboxMin = [0.0 for i in xrange(3)]
		self.bboxMax = [0.0 for i in xrange(3)]
		n = len(data)
		for i in xrange(n):
			v = data[i]
			if i > 0:
				for j in xrange(3):
					self.bboxMin[j] = min(self.bboxMin[j], v[j])
					self.bboxMax[j] = max(self.bboxMax[j], v[j])
			else:
				for j in xrange(3):
					self.bboxMin[j] = v[j]
					self.bboxMax[j] = v[j]
		scl = [self.bboxMax[i] - self.bboxMin[i] for i in xrange(3)]
		qscl = 0xFFFF
		for i in xrange(3):
			if scl[i] != 0.0:
				self.mask |= 1 << i
				scl[i] = float(qscl) / scl[i]
		self.data = []
		for v in data:
			rel = [(v[i] - self.bboxMin[i]) * scl[i] for i in xrange(3)]
			for i in xrange(3):
				if self.mask & (1 << i):
					self.data.append(max(min(int(round(rel[i])), qscl), 0))

	def write(self, bw):
		bw.writeFV(self.bboxMin)
		bw.writeFV(self.bboxMax)
		attr = self.mask & 7
		bw.writeU32(attr)
		for qval in self.data:
			bw.writeU16(qval)

class MotNode:
	def __init__(self, mot, name, nfrm, rotOrd = "xyz"):
		self.mot = mot
		self.name = name
		self.nameId = -1
		self.rotOrd = rotOrd
		self.nfrm = nfrm
		self.trkQ = None
		self.trkT = None
		self.hasR = False
		self.hasT = False

	def encodeTracks(self):
		if self.hasR:
			qdata = []
			for i in xrange(self.nfrm):
				rx = self.rx[i]
				ry = self.ry[i]
				rz = self.rz[i]
				q = xhou.qrot((rx, ry, rz), self.rotOrd)
				v = xcore.quatGetSV(q, False)
				qdata.append(v)
			self.trkQ = MotTrk(qdata)
		if self.hasT:
			tdata = []
			for i in xrange(self.nfrm):
				tdata.append([self.tx[i], self.ty[i], self.tz[i]])
			self.trkT = MotTrk(tdata)

	def write(self, bw):
		attr = xcore.rotOrdFromStr(self.rotOrd)
		self.mot.writeStrId16(bw, self.nameId)
		bw.writeI16(attr)
		bw.writeU32(0) # +04 -> trkQ
		bw.writeU32(0) # +08 -> trkT
		bw.writeU32(0) # +0C -> trkS

class MotExporter(xcore.BaseExporter):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		self.sig = "XMOT"

	def build(self, chop):
		self.chop = chop
		if not chop: return
		self.fps = chop.sampleRate()
		self.srange = chop.sampleRange()
		self.fstart = int(chop.samplesToFrame(self.srange[0]))
		self.fend = int(chop.samplesToFrame(self.srange[1]))
		self.nfrm = self.fend - self.fstart + 1
		self.setNameFromPath(chop.path())
		self.nodes = {}
		zdat = [0.0 for i in xrange(self.nfrm)]
		for itrk, trk in enumerate(chop.tracks()):
			trkName = trk.name()
			chSep = trkName.rfind(":")
			chName = trkName[chSep+1:]
			nodePath = trkName[:chSep]
			nodeName =  nodePath[nodePath.rfind("/")+1:]
			if not nodeName in self.nodes:
				node = MotNode(self, nodeName, self.nfrm)
				node.tx = zdat
				node.ty = zdat
				node.tz = zdat
				node.rx = zdat
				node.ry = zdat
				node.rz = zdat
				self.nodes[nodeName] = node
			node = self.nodes[nodeName]
			node.nameId = self.strLst.add(nodeName)
			data = []
			for ifrm in xrange(self.fstart, self.fend+1):
				# looping over frame range and manually calling evalAtFrame
				# seems to be the only way that works properly for sub-frames?
				data.append(trk.evalAtFrame(ifrm))
			if chName == "tx":
				node.hasT = True
				node.tx = data
			elif chName == "ty":
				node.hasT = True
				node.ty = data
			elif chName == "tz":
				node.hasT = True
				node.tz = data
			elif chName == "rx":
				node.hasR = True
				node.rx = data
			elif chName == "ry":
				node.hasR = True
				node.ry = data
			elif chName == "rz":
				node.hasR = True
				node.rz = data
		for nodeName in self.nodes:
			node = self.nodes[nodeName]
			node.encodeTracks()

	def writeHead(self, bw, top):
		if not self.chop: return
		bw.writeF32(self.fps) # +20
		bw.writeU32(self.nfrm) # +24
		bw.writeI32(self.fstart) # +28
		bw.writeU32(len(self.nodes)) # +2C
		self.patchPos = bw.getPos()
		bw.writeU32(0) # +30 -> nodes
		bw.writeU32(0) # +34 reserved
		bw.writeU32(0) # +38 reserved
		bw.writeU32(0) # +3C reserved

	def writeData(self, bw, top):
		if not self.chop: return
		bw.align(0x10)
		nodesTop = bw.getPos()
		bw.patch(self.patchPos, nodesTop - top) # -> nodes
		for nodeName in self.nodes:
			node = self.nodes[nodeName]
			node.write(bw)
		for inode, nodeName in enumerate(self.nodes):
			node = self.nodes[nodeName]
			nodeInfoOffs = nodesTop + inode*0x10
			if node.trkQ:
				bw.align(4)
				bw.patch(nodeInfoOffs + 4, bw.getPos() - top)
				node.trkQ.write(bw)
			if node.trkT:
				bw.align(4)
				bw.patch(nodeInfoOffs + 8, bw.getPos() - top)
				node.trkT.write(bw)

	def save(self, outPath):
		if not self.chop: return
		xcore.BaseExporter.save(self, outPath)

