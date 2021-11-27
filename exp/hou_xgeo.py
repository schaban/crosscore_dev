# Author: Sergey Chaban <sergey.chaban@gmail.com>

import sys
import hou
import os
import imp
import re
import inspect
from math import *
from array import array

import xcore
import xhou

try: xrange
except: xrange = range

def writeBitMask(bw, bits, minIdx, maxIdx):
	span = maxIdx - minIdx
	n = xcore.ceilDiv(span+1, 8)
	bits >>= minIdx
	for i in xrange(n):
		bw.writeU8(bits & 0xFF)
		bits >>= 8

class GroupData:
	def __init__(self, xgeo):
		self.xgeo = xgeo
		self.bits = 0
		self.lst = []
		self.minIdx = -1
		self.maxIdx = -1
		self.bbox = xhou.BBox()
		self.maxWgtNum = 0
		self.skinBits = 0
		self.skinNodes = []

	def addIdx(self, idx):
		if len(self.lst):
			self.minIdx = min(idx, self.minIdx)
			self.maxIdx = max(idx, self.maxIdx)
		else:
			self.minIdx = idx
			self.maxIdx = idx
		self.bits |= 1 << idx
		self.lst.append(idx)

	def getIdxNum(self): return len(self.lst)

	def contains(self, idx):
		return (self.bits & (1<<idx)) != 0

	def getSkinNodeNum(self): return len(self.skinNodes)

	def addSkinNode(self, idx):
		mask = 1<<idx
		if self.skinBits & mask: return
		self.skinBits |= mask
		self.skinNodes.append(idx)
		self.skinNodes.sort()

	def makeDummySkinSpheres(self):
		nskin = self.getSkinNodeNum()
		if nskin <= 0: return
		self.skinSph = []
		for nodeId in self.skinNodes:
			sph = SkinSphere(self.xgeo, nodeId)
			self.skinSph.append(sph)

	def calcPolSkinSpheres(self):
		nskin = self.getSkinNodeNum()
		if nskin <= 0: return
		self.skinSph = []
		for nodeId in self.skinNodes:
			sph = SkinSphere(self.xgeo, nodeId)
			for polNo in self.lst:
				self.xgeo.pols[polNo].updateSkinSphere(sph)
			sph.estimate()
			for polNo in self.lst:
				self.xgeo.pols[polNo].growSkinSphere(sph)
			self.skinSph.append(sph)

	def calcPntSkinSpheres(self):
		nskin = self.getSkinNodeNum()
		if nskin <= 0: return
		self.skinSph = []
		for nodeId in self.skinNodes:
			sph = SkinSphere(self.xgeo, nodeId)
			for ptNo in self.lst:
				self.xgeo.getPntSkin(ptNo).updateSkinSphere(sph)
			sph.estimate()
			for ptNo in self.lst:
				self.xgeo.getPntSkin(ptNo).growSkinSphere(sph)
			self.skinSph.append(sph)

	def write(self, bw):
		bw.writeFV(self.bbox.getMinPos())
		bw.writeFV(self.bbox.getMaxPos())
		nidx = self.getIdxNum()
		nskin = self.getSkinNodeNum()
		bw.writeI32(self.minIdx)
		bw.writeI32(self.maxIdx)
		bw.writeU16(self.maxWgtNum)
		bw.writeU16(nskin)
		bw.writeU32(nidx)
		if nskin > 0:
			if self.xgeo.skinSphFlg:
				for sph in self.skinSph:
					sph.write(bw)
			for nodeId in self.skinNodes:
				bw.writeU16(nodeId)
		if nidx > 0:
			idxSpan = self.maxIdx - self.minIdx
			if idxSpan < (1<<8):
				for idx in self.lst: bw.writeU8(idx - self.minIdx)
			elif idxSpan < (1<<16):
				for idx in self.lst: bw.writeU16(idx - self.minIdx)
			else:
				for idx in self.lst: bw.writeU24(idx - self.minIdx)
			writeBitMask(bw, self.bits, self.minIdx, self.maxIdx)

class MaterialInfo(GroupData):
	def __init__(self, shopPath, xgeo):
		self.shopPath = shopPath
		self.strLst = xgeo.strLst
		self.nameId, self.pathId = self.strLst.addNameAndPath(shopPath)
		GroupData.__init__(self, xgeo)

	def addPol(self, idx): self.addIdx(idx)

	def getName(self): return self.strLst.get(self.nameId)
	def getPath(self): return self.strLst.get(self.pathId)
	def getPolNum(self): return self.getIdxNum()

	def write(self, bw):
		#print self.shopPath, "@", hex(bw.getPos())
		self.xgeo.writeStrId16(bw, self.nameId) # +00
		self.xgeo.writeStrId16(bw, self.pathId) # +02
		GroupData.write(self, bw)

class GroupInfo(GroupData):
	def __init__(self, name, xgeo):
		self.strLst = xgeo.strLst
		self.nameId = self.strLst.add(name)
		GroupData.__init__(self, xgeo)

	def add(self, idx): self.addIdx(idx)

	def getName(self): return self.strLst.get(self.nameId)

	def write(self, bw):
		self.xgeo.writeStrId16(bw, self.nameId) # +00
		bw.writeI16(-1) # +02 empty path
		GroupData.write(self, bw)

class Polygon:
	def __init__(self, xgeo, prim):
		self.xgeo = xgeo
		self.prim = prim
		self.pntLst = []
		self.mtlId = -1
		self.bbox = xhou.BBox()
		for vtx in prim.vertices():
			pnt = vtx.point()
			self.pntLst.append(pnt.number())
			pos = pnt.position()
			self.bbox.addPos(pos)
		self.vtxNum = len(self.pntLst)
		self.grpNum = 0
		self.grpMinIdx = 0
		self.grpMaxIdx = 0
		self.grpMask = 0
		self.grpLst = []

	def addGrpIdx(self, idx):
		if self.grpMask & gbit: return
		gbit = 1 << idx
		if self.grpNum > 0:
			self.grpMinIdx = min(idx, self.grpMinIdx)
			self.grpMaxIdx = max(idx, self.grpMaxIdx)
		else:
			self.grpMinIdx = idx
			self.grpMaxIdx = idx
		self.grpLst.append(idx)
		self.grpNum += 1
		self.grpMask |= gbit

	def getMaxWgtNum(self):
		n = 0
		for ptNo in self.pntLst:
			n = max(n, self.xgeo.getPntWgtNum(ptNo))
		return n

	def addSkinNodesToGrp(self, grp):
		for ptNo in self.pntLst:
			skin = self.xgeo.getPntSkin(ptNo)
			skin.addNodesToGrp(grp)

	def updateSkinSphere(self, sph):
		for ptNo in self.pntLst:
			skin = self.xgeo.getPntSkin(ptNo)
			skin.updateSkinSphere(sph)

	def growSkinSphere(self, sph):
		for ptNo in self.pntLst:
			skin = self.xgeo.getPntSkin(ptNo)
			skin.growSkinSphere(sph)

	def writeExtInfo(self, bw):
		bw.writeU16(self.grpNum)
		bw.writeU16(self.grpMinIdx)
		bw.writeU16(self.grpMaxIdx)
		grpSpan = self.grpMaxIdx - self.grpMinIdx
		if grpSpan:
			if grpSpan < (1<<8):
				for gidx in self.grpLst: bw.writeU8(gidx - self.grpMinIdx)
			else:
				for gidx in self.grpLst: bw.writeU16(gidx - self.grpMinIdx)

	def write(self, bw):
		if self.xgeo.pntNum <= (1<<8):
			for ptNo in self.pntLst: bw.writeU8(ptNo)
		elif self.xgeo.pntNum <= (1<<16):
			for ptNo in self.pntLst: bw.writeU16(ptNo)
		else:
			for ptNo in self.pntLst: bw.writeU24(ptNo)

class PntSkin(xhou.PntSkin):
	def __init__(self, xgeo, pnt, skinAttr):
		self.xgeo = xgeo
		xhou.PntSkin.__init__(self, pnt, skinAttr)

	def addNodesToGrp(self, grp):
		for idx in self.idx: grp.addSkinNode(idx)

	def updateSkinSphere(self, sph):
		ptNo = self.pnt.number()
		for idx in self.idx:
			if idx == sph.nodeId:
				sph.update(ptNo)
				break

	def growSkinSphere(self, sph):
		ptNo = self.pnt.number()
		pos = self.xgeo.getPntPos(ptNo)
		for idx in self.idx:
			if idx == sph.nodeId:
				sph.grow(pos)
				break

class SkinSphere:
	def __init__(self, xgeo, nodeId):
		self.xgeo = xgeo
		self.nodeId = nodeId
		self.imin = [-1, -1, -1]
		self.imax = [-1, -1, -1]
		self.center = xcore.vecZero()
		self.radius = 0.0

	def update(self, ptNo):
		pos = self.xgeo.getPntPos(ptNo)
		for i in xrange(3):
			if self.imin[i] < 0 or pos[i] < self.xgeo.getPntPos(self.imin[i])[i]: self.imin[i] = ptNo
			if self.imax[i] < 0 or pos[i] > self.xgeo.getPntPos(self.imax[i])[i]: self.imax[i] = ptNo

	def estimate(self):
		d2x = xcore.vecMag2(self.xgeo.getPntPos(self.imax[0]) - self.xgeo.getPntPos(self.imin[0]))
		d2y = xcore.vecMag2(self.xgeo.getPntPos(self.imax[1]) - self.xgeo.getPntPos(self.imin[1]))
		d2z = xcore.vecMag2(self.xgeo.getPntPos(self.imax[2]) - self.xgeo.getPntPos(self.imin[2]))
		minIdx = self.imin[0]
		maxIdx = self.imax[0]
		if d2y > d2x and d2y > d2z:
			minIdx = self.imin[1]
			maxIdx = self.imax[1]
		if d2z > d2x and d2z > d2y:
			minIdx = self.imin[2]
			maxIdx = self.imax[2]
		maxPos = self.xgeo.getPntPos(maxIdx)
		self.center = (self.xgeo.getPntPos(minIdx) + maxPos) * 0.5
		self.radius = xcore.vecMag(maxPos - self.center)

	def grow(self, pos):
		v = pos - self.center
		dist2 = xcore.vecMag2(v)
		if dist2 > xcore.sq(self.radius):
			dist = sqrt(dist2)
			r = (self.radius + dist) * 0.5
			s = xcore.div0((r - self.radius), dist)
			self.radius = r
			self.center += v * s

	def write(self, bw):
		bw.writeFV(self.center)
		bw.writeF32(self.radius)


class BVHNode:
	def __init__(self, bvh):
		self.bvh = bvh
		self.bbox = None
		self.polIdLst = []
		self.left = None
		self.right = None
		self.lvl = -1
		self.bvh.addNode(self)

	def addPols(self, idx, count):
		self.bbox = xhou.BBox()
		for i in xrange(count):
			self.bbox.addBox(self.bvh.getPolBBox(idx+i))
			self.polIdLst.append(self.bvh.getPolId(idx+i))

	def build(self, idx, count, axis, lvl):
		self.lvl = lvl
		if count == 1:
			self.addPols(idx, count)
		elif count == 2:
			self.left = BVHNode(self.bvh)
			self.left.addPols(idx, 1)
			self.left.lvl = lvl + 1
			self.right = BVHNode(self.bvh)
			self.right.addPols(idx+1, 1)
			self.bbox = xhou.BBox()
			self.bbox.addBox(self.left.bbox)
			self.bbox.addBox(self.right.bbox)
			self.right.lvl = lvl + 1
		else:
			self.bbox = xhou.BBox()
			for i in xrange(count):
				self.bbox.addBox(self.bvh.getPolBBox(idx+i))
			mid = self.bvh.split(idx, count, self.bbox.getCenter()[axis], axis)
			nextAxis = (axis + 1) % 3
			self.left = BVHNode(self.bvh)
			self.right = BVHNode(self.bvh)
			self.left.build(idx, mid, nextAxis, lvl + 1)
			self.right.build(idx + mid, count - mid, nextAxis, lvl + 1)

	def write(self, bw):
		bw.writeFV(self.bbox.getMinPos())
		bw.writeFV(self.bbox.getMaxPos())
		polId = -1
		left = -1
		right = -1
		if len(self.polIdLst): polId = self.polIdLst[0]
		if self.left: left = self.bvh.nodeMap[self.left]
		if self.right: right = self.bvh.nodeMap[self.right]
		if right < 0:
			bw.writeI32(polId)
		else:
			bw.writeI32(left)
		bw.writeI32(right)

class BVH:
	def __init__(self, xgeo):
		self.xgeo = xgeo
		self.bbox = xhou.BBox()
		self.bbox.fromHouBBox(self.xgeo.bbox)
		npol = self.xgeo.polNum
		self.polIds = [i for i in xrange(npol)]
		self.nodeLst = []
		self.nodeMap = {}
		self.root = BVHNode(self)
		self.root.build(0, npol, self.bbox.getMaxAxis(), 0)

	def addNode(self, node):
		self.nodeMap[node] = len(self.nodeLst)
		self.nodeLst.append(node)

	def getPolId(self, idx): return self.polIds[idx]

	def getPolBBox(self, idx):
		polId = self.getPolId(idx)
		pol = self.xgeo.pols[polId]
		return pol.bbox

	def split(self, idx, count, pivot, axis):
		mid = 0
		for i in xrange(count):
			cent = self.xgeo.pols[self.polIds[idx + i]].bbox.getCenter()[axis]
			if cent < pivot:
				t = self.polIds[idx + i]
				self.polIds[idx + i] = self.polIds[idx + mid]
				self.polIds[idx + mid] = t
				mid += 1
		if mid == 0 or mid == count: mid = count // 2
		return mid

	def write(self, bw):
		bw.writeU32(len(self.nodeLst))
		bw.writeU32(0)
		bw.writeU32(0)
		bw.writeU32(0)
		for node in self.nodeLst: node.write(bw)

def getAttrType(attr):
	return [hou.attribData.Int, hou.attribData.Float, hou.attribData.String].index(attr.dataType())

class GeoExporter(xcore.BaseExporter):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		self.sig = "XGEO"

	def build(self, sop, bvhFlg = not True, skinSphFlg = False):
		self.sop = sop
		self.bvhFlg = bvhFlg
		self.skinSphFlg = skinSphFlg # write skin spheres for all groups
		self.geo = sop.geometry()
		self.bbox = self.geo.boundingBox()
		self.setNameFromPath(self.sop.parent().path())
		self.pntNum = len(self.geo.points())
		self.pnts = []
		for pnt in self.geo.points():
			self.pnts.append(pnt.position())
		self.pols = []
		self.primTbl = []
		self.maxVtxPerPol = 0
		hasQuads = False
		for prim in self.geo.prims():
			if prim.type() == hou.primType.Polygon:
				self.primTbl.append(len(self.pols))
				pol = Polygon(self, prim)
				self.maxVtxPerPol = max(pol.vtxNum, self.maxVtxPerPol)
				self.pols.append(pol)
				if pol.vtxNum == 4: hasQuads = True
			else:
				self.primTbl.append(-1)
		self.polNum = len(self.pols)
		self.samePolSize = False
		if self.polNum:
			nvtx = self.pols[0].vtxNum
			self.samePolSize = True
			for pol in self.pols:
				if pol.vtxNum != nvtx:
					self.samePolSize = False
					break

		self.allQuadsPlanarConvex = False
		if hasQuads:
			planarEps = 1.0e-6
			foundNonPlanar = False
			foundNonCovex = False
			for pol in self.pols:
				if pol.vtxNum == 4:
					polP0 = pol.prim.vertex(0).point().position()
					polP1 = pol.prim.vertex(1).point().position()
					polP2 = pol.prim.vertex(2).point().position()
					polP3 = pol.prim.vertex(3).point().position()
					polCen = (polP0 + polP1 + polP2 + polP3) / 4
					polNrm = pol.prim.normal()
					polD = polCen.dot(polNrm)
					for vtx in pol.prim.vertices():
						vtxPos = vtx.point().position()
						vtxDist = abs(vtxPos.dot(polNrm) - polD)
						if vtxDist > planarEps:
							foundNonPlanar = True
							break
					if not foundNonPlanar:
						polE0 = polP1 - polP0
						polE1 = polP2 - polP1
						polE2 = polP3 - polP2
						polE3 = polP0 - polP3
						cvxMask = 0
						polN01 = polE0.cross(polE1)
						polN23 = polE2.cross(polE3)
						if polN01.dot(polN23) > 0: cvxMask |= 1
						polN12 = polE1.cross(polE2)
						polN30 = polE3.cross(polE0)
						if polN12.dot(polN30) > 0: cvxMask |= 2
						if cvxMask != 3: foundNonCovex = True
				if foundNonPlanar or foundNonCovex: break
			self.allQuadsPlanarConvex = not foundNonPlanar and not foundNonCovex

		self.skinFlg = False
		self.skinNodeNum = 0
		self.maxSkinWgt = 0
		self.skinNames = xhou.getSkinNames(self.geo)
		if self.skinNames:
			self.skinIds = []
			for sn in self.skinNames:
				self.skinIds.append(self.strLst.add(sn))
			self.skinLst = []
			skinAttr = xhou.getSkinAttr(self.geo)
			for pnt in self.geo.points():
				skin = PntSkin(self, pnt, skinAttr)
				self.maxSkinWgt = max(skin.getWgtNum(), self.maxSkinWgt)
				self.skinLst.append(skin)
			#xcore.dbgmsg("max wgt: " + self.maxSkinWgt)
			if self.maxSkinWgt:
				self.skinFlg = True
				self.skinNodeNum = len(self.skinNames)
			if self.skinFlg:
				self.glbSkinSph = []
				for nodeId in xrange(self.skinNodeNum):
					sph = SkinSphere(self, nodeId)
					for skin in self.skinLst:
						skin.updateSkinSphere(sph)
					sph.estimate()
					for skin in self.skinLst:
						skin.growSkinSphere(sph)
					self.glbSkinSph.append(sph)

		validDataTypes = [hou.attribData.Int, hou.attribData.Float, hou.attribData.String]

		glbAttrExclude = ["pCaptSkelRoot", "pCaptFrame", "varmap"]
		self.glbAttrs = []
		for attr in self.geo.globalAttribs():
			if not attr.name() in glbAttrExclude and attr.dataType() in validDataTypes:
				self.glbAttrs.append(attr)

		pntAttrExclude = ["P", "Pw", "pCapt", "boneCapture"]
		self.pntAttrs = []
		for attr in self.geo.pointAttribs():
			if not attr.name() in pntAttrExclude and attr.dataType() in validDataTypes:
				self.pntAttrs.append(attr)

		polAttrExclude = ["shop_materialpath"]
		self.polAttrs = []
		for attr in self.geo.primAttribs():
			if not attr.name() in polAttrExclude and attr.dataType() in validDataTypes:
				self.polAttrs.append(attr)

		self.mtlNum = 0
		self.samePolMtl = True
		if self.geo.findPrimAttrib("shop_materialpath"):
			self.mtlFlg = True
			self.mtlLst = []
			self.mtlMap = {}
			for pol in self.pols:
				shopPath = pol.prim.attribValue("shop_materialpath")
				if not shopPath in self.mtlMap:
					mtlId = len(self.mtlLst)
					mtl = MaterialInfo(shopPath, self)
					self.mtlMap[shopPath] = mtlId
					self.mtlLst.append(mtl)
			self.mtlNum = len(self.mtlMap)
			for i, pol in enumerate(self.pols):
				shopPath = pol.prim.attribValue("shop_materialpath")
				mtlId = self.mtlMap[shopPath]
				mtl = self.mtlLst[mtlId]
				mtl.addPol(i)
				mtl.bbox.addBox(pol.bbox)
				pol.mtlId = mtlId
			mtlId = self.pols[0].mtlId
			for pol in self.pols:
				if pol.mtlId != mtlId:
					self.samePolMtl = False
					break
			if self.skinFlg:
				for mtl in self.mtlLst:
					maxWgtNum = 0
					for polNo in mtl.lst:
						maxWgtNum = max(maxWgtNum, self.pols[polNo].getMaxWgtNum())
						self.pols[polNo].addSkinNodesToGrp(mtl)
					mtl.maxWgtNum = maxWgtNum
					if self.skinSphFlg: mtl.calcPolSkinSpheres()
					#xcore.dbgmsg("maxWgt/Mtl: " + mtl.getName() + " " + str(mtl.maxWgtNum))
		#self.printMtlInfo()

		self.pntGrp = []
		for grp in self.geo.pointGroups():
			g = GroupInfo(grp.name(), self)
			for pnt in grp.points():
				g.bbox.addPos(pnt.position())
				g.add(pnt.number())
			self.pntGrp.append(g)
		self.pntGrpNum = len(self.pntGrp)
		if self.pntGrpNum > 0 and self.skinFlg:
			for g in self.pntGrp:
				maxWgtNum = 0
				for ptNo in g.lst:
					maxWgtNum = max(maxWgtNum, self.getPntWgtNum(ptNo))
					self.getPntSkin(ptNo).addNodesToGrp(g)
				g.maxWgtNum = maxWgtNum
				if self.skinSphFlg: g.calcPntSkinSpheres()

		self.polGrp = []
		for grp in self.geo.primGroups():
			g = GroupInfo(grp.name(), self)
			for prim in grp.prims():
				if prim.type() == hou.primType.Polygon:
					polIdx = self.primTbl[prim.number()]
					g.add(polIdx)
					g.bbox.addBox(self.pols[polIdx].bbox)
			if g.getIdxNum(): self.polGrp.append(g)
		self.polGrpNum = len(self.polGrp)
		if self.polGrpNum > 0 and self.skinFlg:
			for g in self.polGrp:
				maxWgtNum = 0
				for polNo in g.lst:
					maxWgtNum = max(maxWgtNum, self.pols[polNo].getMaxWgtNum())
					self.pols[polNo].addSkinNodesToGrp(g)
				g.maxWgtNum = maxWgtNum
				if self.skinSphFlg: g.calcPolSkinSpheres()
		for i, g in enumerate(self.polGrp):
			for polIdx in g.lst:
				self.pols[polIdx].grpMask |= 1 << i
				self.pols[polIdx].grpNum += 1

		#xcore.dbgmsg("#pnt grp: " + str(self.pntGrpNum))
		#xcore.dbgmsg("#pol grp: " +str(self.polGrpNum))

		self.addAttrStrings(self.glbAttrs)
		self.addAttrStrings(self.pntAttrs)
		self.addAttrStrings(self.polAttrs)

		if self.polNum <= 0: self.bvhFlg = False
		if self.bvhFlg:
			self.bvh = BVH(self)

	def printMtlInfo(self):
		if self.mtlNum > 0:
			npol = 0
			for i, mtl in enumerate(self.mtlLst):
				xcore.dbgmsg("-- Mtl[" + str(i) + "]")
				xcore.dbgmsg(" Name: " + mtl.getName())
				xcore.dbgmsg(" Path: " + mtl.getPath())
				xcore.dbgmsg(" #pol: " + str(mtl.getPolNum()))
				npol += mtl.getPolNum()
			xcore.dbgmsg("total pol # " + str(npol))

	def addAttrStrings(self, attrLst):
		for attrIdx, attr in enumerate(attrLst):
			self.strLst.add(attr.name())
			if attr.dataType() == hou.attribData.String:
				atype = attr.type()
				n = 0
				if atype == hou.attribType.Point: n = self.pntNum
				elif atype == hou.attribType.Prim: n = self.polNum
				elif atype == hou.attribType.Global: n = 1
				for i in xrange(n):
					if atype == hou.attribType.Point:
						obj = self.geo.iterPoints()[i]
					elif atype == hou.attribType.Prim:
						obj = self.pols[i].prim
					elif atype == hou.attribType.Global:
						obj = self.geo
					self.strLst.add(obj.stringAttribValue(attr))


	#def getPntPos(self, ptNo): return self.geo.iterPoints()[ptNo].position()
	def getPntPos(self, ptNo): return self.pnts[ptNo]

	def getPntWgtNum(self, ptNo): return self.skinLst[ptNo].getWgtNum()
	def getPntSkin(self, ptNo): return self.skinLst[ptNo]

	def writeHead(self, bw, top):
		bw.writeFV(self.bbox.minvec()) # +20
		bw.writeFV(self.bbox.maxvec()) # +2C
		bw.writeU32(self.pntNum) # +38
		bw.writeU32(self.polNum) # +3C
		bw.writeU32(self.mtlNum) # +40
		bw.writeU32(len(self.glbAttrs)) # +44
		bw.writeU32(len(self.pntAttrs)) # +48
		bw.writeU32(len(self.polAttrs)) # +4C
		bw.writeU32(self.pntGrpNum) # +50
		bw.writeU32(self.polGrpNum) # +54
		bw.writeU32(self.skinNodeNum) # +58
		bw.writeU16(self.maxSkinWgt) # +5C
		bw.writeU16(self.maxVtxPerPol) # +5E
		self.patchPos = bw.getPos()
		bw.writeU32(0) # +60 +00 -> pnt[]
		bw.writeU32(0) # +64 +04 -> pol[]
		bw.writeU32(0) # +68 +08 -> mtl[]
		bw.writeU32(0) # +6C +0C -> glbAttr
		bw.writeU32(0) # +70 +10 -> pntAttr
		bw.writeU32(0) # +74 +14 -> polAttr
		bw.writeU32(0) # +78 +18 -> pntGrp
		bw.writeU32(0) # +7C +1C -> polGrp
		bw.writeU32(0) # +80 +20 -> skinNodes
		bw.writeU32(0) # +84 +24 -> wgt
		bw.writeU32(0) # +88 +28 -> BVH

	def writePntData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos, bw.getPos() - top) # -> pnt
		for pnt in self.pnts: bw.writeFV(pnt)

	def writePolData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 4, bw.getPos() - top) # -> pol
		polLstTop = bw.getPos()
		if not self.samePolSize:
			for i in xrange(self.polNum): bw.writeU32(0)
		if not self.samePolMtl:
			for pol in self.pols:
				if self.mtlNum < (1<<7):
					bw.writeI8(pol.mtlId)
				else:
					bw.writeI16(pol.mtlId)
		if not self.samePolSize:
			for pol in self.pols:
				if self.maxVtxPerPol < (1<<8):
					bw.writeU8(pol.vtxNum)
				else:
					bw.writeU16(pol.vtxNum)
		for i, pol in enumerate(self.pols):
			if not self.samePolSize:
				bw.patch(polLstTop + i*4, bw.getPos() - top)
			pol.write(bw)

	def writeMtlData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 8, bw.getPos() - top) # -> mtl
		mtlLstTop = bw.getPos()
		for i in xrange(self.mtlNum): bw.writeU32(0)
		for i, mtl in enumerate(self.mtlLst):
			bw.align(4)
			bw.patch(mtlLstTop + i*4, bw.getPos() - top)
			mtl.write(bw)

	def writePntGrpData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 0x18, bw.getPos() - top) # -> pntGrp
		lstTop = bw.getPos()
		for i in xrange(self.pntGrpNum): bw.writeU32(0)
		for i, g in enumerate(self.pntGrp):
			bw.align(4)
			bw.patch(lstTop + i*4, bw.getPos() - top)
			g.write(bw)

	def writePolGrpData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 0x1C, bw.getPos() - top) # -> polGrp
		lstTop = bw.getPos()
		for i in xrange(self.polGrpNum): bw.writeU32(0)
		for i, g in enumerate(self.polGrp):
			bw.align(4)
			bw.patch(lstTop + i*4, bw.getPos() - top)
			g.write(bw)

	def writeAttrVal(self, bw, attr, obj):
		type = attr.dataType()
		if type == hou.attribData.Int:
			val = obj.intListAttribValue(attr)
			for x in val: bw.writeI32(x)
		elif type == hou.attribData.Float:
			val = obj.floatListAttribValue(attr)
			for x in val: bw.writeF32(x)
		elif type == hou.attribData.String:
			self.writeStrId32(bw, self.strLst.getIdx(obj.stringAttribValue(attr)))

	def writeAttrData(self, bw, top, attrLst):
		attrPatchTop = bw.getPos()
		for attr in attrLst:
			bw.writeU32(0) # +00 -> data
			self.writeStrId32(bw, self.strLst.getIdx(attr.name())) # +04
			bw.writeU16(attr.size()) # +08 len
			bw.writeU8(getAttrType(attr)) # +0A type
			bw.writeU8(0) # +0B reserved
		for attrIdx, attr in enumerate(attrLst):
			atype = attr.type()
			n = 0
			if atype == hou.attribType.Point: n = self.pntNum
			elif atype == hou.attribType.Prim: n = self.polNum
			elif atype == hou.attribType.Global: n = 1
			bw.align(4)
			bw.patch(attrPatchTop + (attrIdx*0xC), bw.getPos() - top)
			for i in xrange(n):
				if atype == hou.attribType.Point:
					pnt = self.geo.iterPoints()[i]
					self.writeAttrVal(bw, attr, pnt)
				elif atype == hou.attribType.Prim:
					pol = self.pols[i].prim
					self.writeAttrVal(bw, attr, pol)
				elif atype == hou.attribType.Global:
					self.writeAttrVal(bw, attr, self.geo)

	def writeAttrs(self, bw, top):
		if len(self.glbAttrs) > 0:
			bw.align(4)
			bw.patch(self.patchPos + 0xC, bw.getPos() - top)
			self.writeAttrData(bw, top, self.glbAttrs)
		if len(self.pntAttrs) > 0:
			bw.align(4)
			bw.patch(self.patchPos + 0x10, bw.getPos() - top)
			self.writeAttrData(bw, top, self.pntAttrs)
		if len(self.polAttrs) > 0:
			bw.align(4)
			bw.patch(self.patchPos + 0x14, bw.getPos() - top)
			self.writeAttrData(bw, top, self.polAttrs)

	def writeSkin(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 0x20, bw.getPos() - top) # -> skin nodes
		for sph in self.glbSkinSph: sph.write(bw) # spheres
		for id in self.skinIds: self.writeStrId32(bw, id) # names
		bw.patch(self.patchPos + 0x24, bw.getPos() - top) # -> wgt
		n = self.pntNum
		offsTop = bw.getPos()
		for i in xrange(n): bw.writeU32(0)
		for i in xrange(n): bw.writeU8(self.skinLst[i].getWgtNum())
		nodeNum = len(self.skinIds)
		for i in xrange(n):
			bw.align(4)
			bw.patch(offsTop + (i*4), bw.getPos() - top)
			wnum = self.skinLst[i].getWgtNum()
			for j in xrange(wnum): bw.writeF32(self.skinLst[i].wgt[j])
			for j in xrange(wnum):
				if nodeNum <= (1<<8):
					bw.writeU8(self.skinLst[i].idx[j])
				else:
					bw.writeU16(self.skinLst[i].idx[j])

	def writeBVH(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 0x28, bw.getPos() - top)
		self.bvh.write(bw)

	def writeData(self, bw, top):
		if self.pntNum > 0: self.writePntData(bw, top)
		if self.polNum > 0: self.writePolData(bw, top)
		if self.mtlNum > 0: self.writeMtlData(bw, top)
		if self.pntGrpNum > 0: self.writePntGrpData(bw, top)
		if self.polGrpNum > 0: self.writePolGrpData(bw, top)
		self.writeAttrs(bw, top)
		if self.skinFlg: self.writeSkin(bw, top)
		if self.bvhFlg: self.writeBVH(bw, top)

	def save(self, outPath):
		if self.samePolSize: self.flags |= 1
		if self.samePolMtl: self.flags |= 2
		if self.skinFlg and self.skinSphFlg: self.flags |= 4
		if self.allQuadsPlanarConvex: self.flags |= 8
		xcore.BaseExporter.save(self, outPath)



