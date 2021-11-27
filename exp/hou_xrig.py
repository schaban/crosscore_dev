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

def vecAll(vec, val):
	for elem in vec:
		if elem != val: return False
	return True

class RigNode:
	def __init__(self, xrig, hnode, lvl):
		self.xrig = xrig
		self.hnode = hnode
		self.lvl = lvl
		self.wmtx = hnode.worldTransform()
		self.lmtx = hnode.localTransform()
		self.path = hnode.path()
		self.name = hnode.name()
		self.type = hnode.type().name()
		self.attr = 0
		inp = hnode.inputConnectors()[0]
		if len(inp):
			self.parent = inp[0].inputNode()
		else:
			self.parent = None
		self.rordStr = xhou.getRotOrd(self.hnode)
		self.xordStr = xhou.getXformOrd(self.hnode)
		self.rord = xcore.rotOrdFromStr(self.rordStr)
		self.xord = xcore.xformOrdFromStr(self.xordStr)
		loc = self.lmtx.explode(self.xordStr, self.rordStr)
		self.lpos = loc["translate"]
		self.lrot = loc["rotate"]
		self.lscl = loc["scale"]

	def writeInfo(self, bw):
		bw.writeI16(self.selfIdx)
		bw.writeI16(self.parentIdx)
		self.xrig.writeStrId16(bw, self.nameId)
		self.xrig.writeStrId16(bw, self.pathId)
		self.xrig.writeStrId16(bw, self.typeId)
		bw.writeI16(self.lvl)
		bw.writeU16(self.attr)
		bw.writeU8(self.rord)
		bw.writeU8(self.xord)

class RigExporter(xcore.BaseExporter):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		self.sig = "XRIG"
		self.infoLst = []

	def findNode(self, nodeName):
		nodeId = -1
		if self.nodeMap and nodeName in self.nodeMap:
			nodeId = self.nodeMap[nodeName]
		return nodeId

	def build(self, rootPath):
		self.nodeMap = {}
		self.nodes = []
		self.buildSub(hou.node(rootPath), 0)
		self.maxLvl = 0
		self.defRot = True
		self.defScl = True
		for i, node in enumerate(self.nodes):
			node.selfIdx = i
			self.maxLvl = max(self.maxLvl, node.lvl)
			node.typeId = self.strLst.add(node.type)
			node.nameId = self.strLst.add(node.name)
			node.pathId = -1
			sep = node.path.rfind("/")
			if sep >= 0: node.pathId = self.strLst.add(node.path[:sep])
			if node.parent:
				node.parentIdx = self.nodeMap[node.parent.name()]
			else:
				node.parentIdx = -1
			if self.defRot:
				if not vecAll(node.lrot, 0.0):
					self.defRot = False
			if self.defScl:
				if not vecAll(node.lscl, 1.0):
					self.defScl = False

	def buildSub(self, hnode, lvl):
		self.nodeMap[hnode.name()] = len(self.nodes)
		self.nodes.append(RigNode(self, hnode, lvl))
		for link in hnode.outputConnectors()[0]:
			self.buildSub(link.outputNode(), lvl+1)

	def addInfo(self, info):
		self.infoLst.append(info)

	def writeHead(self, bw, top):
		bw.writeU32(len(self.nodes)) # +20 nodeNum
		bw.writeU32(self.maxLvl + 1) # +24 lvlNum
		self.patchPos = bw.getPos()
		bw.writeU32(0) # +28 : +00 -> info[]
		bw.writeU32(0) # +2C : +04 -> wmtx[]
		bw.writeU32(0) # +30 : +08 -> imtx[]
		bw.writeU32(0) # +34 : +0C -> lmtx[]
		bw.writeU32(0) # +38 : +10 -> lpos[]
		bw.writeU32(0) # +3C : +14 -> lrot[]
		bw.writeU32(0) # +40 : +18 -> lscl[]
		bw.writeU32(0) # +44 : +1C -> info

	def writeData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos, bw.getPos() - top)
		for node in self.nodes:
			node.writeInfo(bw)
		bw.align(0x10)
		bw.patch(self.patchPos + 4, bw.getPos() - top)
		for node in self.nodes:
			bw.writeFV(node.wmtx.asTuple())
		bw.patch(self.patchPos + 8, bw.getPos() - top)
		for node in self.nodes:
			bw.writeFV(node.wmtx.inverted().asTuple())
		bw.patch(self.patchPos + 0xC, bw.getPos() - top)
		for node in self.nodes:
			bw.writeFV(node.lmtx.asTuple())
		bw.patch(self.patchPos + 0x10, bw.getPos() - top)
		for node in self.nodes:
			bw.writeFV(node.lpos)
		if not self.defRot:
			bw.patch(self.patchPos + 0x14, bw.getPos() - top)
			for node in self.nodes:
				bw.writeFV(node.lrot)
		if not self.defScl:
			bw.patch(self.patchPos + 0x18, bw.getPos() - top)
			for node in self.nodes:
				bw.writeFV(node.lscl)
		ninf = len(self.infoLst)
		if ninf > 0:
			infoOffs = bw.getPos() - top
			bw.patch(self.patchPos + 0x1C, infoOffs)
			bw.writeFOURCC("LIST")
			bw.writeU32(infoOffs + 0x10)
			bw.writeU32(ninf)
			bw.writeU32(0)
			for i in xrange(ninf):
				self.infoLst[i].writeHead(bw, top, i)
			for i in xrange(ninf):
				self.infoLst[i].writeData(bw, top)

