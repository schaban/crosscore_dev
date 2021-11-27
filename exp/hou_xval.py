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

class ValType:
	def __init__(self): pass
ValType.UNKNOWN = 0
ValType.FLOAT = 1
ValType.VEC2 = 2
ValType.VEC3 = 3
ValType.VEC4 = 4
ValType.INT = 5
ValType.STRING = 6

class PrmVecList:
	def __init__(self):
		self.lst = []

	def add(self, vec):
		id = len(self.lst)
		self.lst.append(vec)
		return id

	def get(self, idx):
		return self.lst[idx]

	def num(self):
		return len(self.lst)

class ParamInfo:
	def __init__(self, xval, prm, fnameFlg = False):
		self.xval = xval
		strLst = xval.strLst
		v2Lst = xval.v2Lst
		v3Lst = xval.v3Lst
		v4Lst = xval.v4Lst
		pt = prm.tuple()
		self.prm = prm
		self.name = pt.name()
		self.nameId = strLst.add(self.name)
		self.type = ValType.UNKNOWN
		self.valId = -1
		t = prm.parmTemplate().type()
		if t == hou.parmTemplateType.Toggle or t == hou.parmTemplateType.Int:
			self.type = ValType.INT
			self.valId = prm.evalAsInt()
		elif t == hou.parmTemplateType.Float:
			if len(pt) > 1:
				if len(pt) == 2:
					self.type = ValType.VEC2
					self.valId = v2Lst.add(pt.evalAsFloats())
				elif len(pt) == 3:
					self.type = ValType.VEC3
					self.valId = v3Lst.add(pt.evalAsFloats())
				elif len(pt) == 4:
					self.type = ValType.VEC4
					self.valId = v4Lst.add(pt.evalAsFloats())
			else:
				self.type = ValType.FLOAT
				self.valId = prm.evalAsFloat()
		elif t == hou.parmTemplateType.String or t == hou.parmTemplateType.Menu:
			if t == hou.parmTemplateType.String:
				prmKeys = xhou.getParamKeys(prm)
				exprFlg = False
				if not prmKeys:
					exprFlg = prm.unexpandedString().startswith("`") or prm.unexpandedString().startswith("op:")
				if exprFlg or prmKeys:
					str = prm.evalAsString()
				else:
					str = prm.unexpandedString() # to keep $HIP and the like
			else:
				str = prm.evalAsString()
			if fnameFlg:
				if prm.parmTemplate().stringType() == hou.stringParmType.FileReference:
					str = xhou.texPathToName(str)
			self.type = ValType.STRING
			self.valId = strLst.add(str)
		elif t == hou.parmTemplateType.Folder:
			if prm.parmTemplate().folderType() == hou.folderType.MultiparmBlock:
				self.type = ValType.INT
				self.valId = prm.evalAsInt()

	def write(self, bw):
		self.xval.writeStrId16(bw, self.nameId)
		bw.writeI8(self.type)
		bw.writeI8(0) # reserved
		if self.type == ValType.FLOAT:
			bw.writeF32(self.valId)
		elif self.type == ValType.STRING:
			self.xval.writeStrId32(bw, self.valId)
		else:
			bw.writeI32(self.valId)

class ValGrp:
	def __init__(self, xval, node, fnameFlg = False, folders = None):
		self.xval = xval
		strLst = xval.strLst
		v2Lst = xval.v2Lst
		v3Lst = xval.v3Lst
		v4Lst = xval.v4Lst
		self.strLst = strLst
		self.v2Lst = v2Lst
		self.v3Lst = v3Lst
		self.v4Lst = v4Lst
		self.lst = []
		self.node = node
		self.fnameFlg = fnameFlg
		self.name = node.name()
		self.path = node.path()
		self.type = node.type()
		self.pathId = -1
		sep = self.path.rfind("/")
		if sep >= 0: self.pathId = strLst.add(self.path[:sep])
		self.nameId = strLst.add(self.name)
		self.typeId = strLst.add(self.type.name())
		if folders:
			for folder in folders:
				flg = False
				for prm in node.parms():
					if prm.parmTemplate().type() == hou.parmTemplateType.FolderSet:
						if folder in prm.parmTemplate().folderNames():
							flg = True
							break
				if flg:
					for prm in node.parmsInFolder([folder]):
						self.addPrm(prm)
		else:
			for prm in self.node.parms():
				self.addPrm(prm)

	def addPrm(self, prm):
		if not prm.parmTemplate().isHidden():
			if prm.componentIndex() == 0:
				self.lst.append(ParamInfo(self.xval, prm, self.fnameFlg))

	def write(self, bw):
		self.xval.writeStrId16(bw, self.nameId)
		self.xval.writeStrId16(bw, self.pathId)
		self.xval.writeStrId16(bw, self.typeId)
		bw.writeI16(0) # reserved
		bw.writeI32(len(self.lst))
		for prm in self.lst:
			prm.write(bw)

class ValGrpList:
	def __init__(self, xval, nodeLst, fnameFlg = False, folders = None):
		self.xval = xval
		strLst = xval.strLst
		vecLst = xval.vecLst
		v2Lst = xval.v2Lst
		v3Lst = xval.v3Lst
		v4Lst = xval.v4Lst
		self.lst = []
		for node in nodeLst:
			self.lst.append(ValGrp(xval, node, fnameFlg, folders))
		if 0:
			xcore.dbgmsg("#vec2 " + str(v2Lst.num()))
			xcore.dbgmsg("#vec3 " + str(v3Lst.num()))
			xcore.dbgmsg("#vec4 " + str(v4Lst.num()))
			xcore.dbgmsg("# " + str(v2Lst.num() + v3Lst.num() + v4Lst.num()))
		for grp in self.lst:
			for prm in grp.lst:
				if prm.type == ValType.VEC4:
					prm.valId = vecLst.add(v4Lst.get(prm.valId))
		for grp in self.lst:
			for prm in grp.lst:
				if prm.type == ValType.VEC3:
					prm.valId = vecLst.add(v3Lst.get(prm.valId))
		for grp in self.lst:
			for prm in grp.lst:
				if prm.type == ValType.VEC2:
					prm.valId = vecLst.add(v2Lst.get(prm.valId))

	def write(self, bw, top):
		lstTop = bw.getPos()
		bw.writeI32(len(self.lst)) # num groups
		for grp in self.lst: bw.writeI32(0)
		for i, grp in enumerate(self.lst):
			bw.patch(lstTop + 4 + i*4, bw.getPos() - top)
			grp.write(bw)

class ValExporter(xcore.BaseExporter):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		self.sig = "XVAL"

	def build(self, lst, fnameFlg = False, folders = None):
		self.vecLst = xcore.VecList()
		self.v2Lst = PrmVecList()
		self.v3Lst = PrmVecList()
		self.v4Lst = PrmVecList()
		self.grpLst = ValGrpList(self, lst, fnameFlg, folders)

	def writeHead(self, bw, top):
		self.patchPos = bw.getPos()
		bw.writeU32(0) # +20 offsVec
		bw.writeU32(0) # +24 offsGrp

	def writeData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos + 0, bw.getPos() - top) # offsVec
		self.vecLst.write(bw)
		bw.patch(self.patchPos + 4, bw.getPos() - top) # offsGrp
		self.grpLst.write(bw, top)

def expVal(lst, outPath, fnameFlg = False, folders = None):
	val = ValExporter()
	val.build(lst, fnameFlg, folders)
	xcore.dbgmsg("Saving values to " + outPath)
	val.save(outPath)

def expValByType(outPath, typLst, folders = None, rootPath = "/obj"):
	nodes = hou.node(rootPath).children()
	lst = []
	for node in nodes:
		if typLst == None or node.type().name() in typLst:
			lst.append(node)
	expVal(lst, outPath, folders)

def expCamVal(outPath):
	expValByType(outPath, ["cam"])

def expLitVal(outPath):
	expValByType(outPath, ["hlight"])

def expAnyVal(outPath):
	expValByType(outPath, None)

def expNetVal(outPath, folders = None, rootPath = "/obj"):
	nodeLst = xhou.NodeList()
	nodeLst.netBuild(rootPath)
	lst = nodeLst.nodes
	expVal(lst, outPath, folders)

def expMtlVal(outPath, folders = None, rootPath = "/obj", mtlTypes = ["GL_lookdev", "v_layered", "mantrasurface", "principledshader"]):
	nodeLst = xhou.NodeList()
	nodeLst.netBuild(rootPath)
	lst = []
	for node in nodeLst.nodes:
		if node.type().name() in mtlTypes:
			lst.append(node)
	if len(lst):
		expVal(lst, outPath, folders)


