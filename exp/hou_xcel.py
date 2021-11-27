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

class ExprEntry:
	def __init__(self, exprText, nodeName, nodePath = None, chanName = None):
		self.nodeName = nodeName
		self.nodeNameId = -1
		self.nodePath = nodePath
		self.nodePathId = -1
		self.chanName = chanName
		self.chanNameId = -1
		self.exprText = exprText
		e = xcore.Expr()
		e.compile(exprText)
		self.compiled = e

	def addToLib(self, lib):
		self.lib = lib
		if not lib: return
		strLst = lib.strLst
		if self.nodeName: self.nodeNameId = strLst.add(self.nodeName)
		if self.nodePath: self.nodePathId = strLst.add(self.nodePath)
		if self.chanName: self.chanNameId = strLst.add(self.chanName)

	def printInfo(self):
		xcore.dbgmsg(self.exprText + " @ " + self.nodeName + " " + self.nodePath + " " + self.chanName)

	def write(self, bw):
		lib = self.lib
		if not lib: return
		lib.writeStrId16(bw, self.nodeNameId)
		lib.writeStrId16(bw, self.nodePathId)
		lib.writeStrId16(bw, self.chanNameId)
		bw.writeI16(0) # reserved
		self.compiled.write(bw)

class ExprLibExporter(xcore.BaseExporter):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		self.sig = "XCEL"
		self.exprLst = []

	def setLst(self, lst):
		for entry in lst:
			entry.addToLib(self)
			self.exprLst.append(entry)

	def writeHead(self, bw, top):
		nexp = len(self.exprLst)
		bw.writeU32(nexp) # +20
		self.patchPos = bw.getPos()
		bw.writeU32(0) # -> expr[]

	def writeData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos, bw.getPos() - top)
		nexp = len(self.exprLst)
		patchTop = bw.getPos()
		for i in xrange(nexp):
			bw.writeU32(0)
		for i, entry in enumerate(self.exprLst):
			bw.align(0x10)
			bw.patch(patchTop + i*4, bw.getPos() - top)
			entry.write(bw)
			if 0:
				xcore.dbgmsg(">>>> " + entry.nodeName + ":" + entry.chanName)
				entry.compiled.disasm()


def exprLibDisasm(lib):
	if not lib: return
	for i, entry in enumerate(lib.exprLst):
		xcore.dbgmsg("["+str(i)+"] >>>> " + entry.nodeName + ":" + entry.chanName)
		entry.compiled.disasm()

def getDefExprExclList():
	return ["cubic()", "linear()", "qlinear()", "constant()", "bezier()"]

def getExprLibFromHrc(rootPath="/obj/root"):
	lib = None
	hrc = xhou.NodeList()
	hrc.hrcBuild(rootPath)
	excl = getDefExprExclList()
	lst = []
	for node in hrc.nodes:
		for parm in node.parms():
			try: expr = parm.expression()
			except: expr = None
			if expr and not expr in excl:
				nodeName = node.name()
				nodePath = None
				fullPath = node.path()
				sep = fullPath.rfind("/")
				if sep >= 0: nodePath = fullPath[:sep]
				chanName = parm.name()
				ent = ExprEntry(expr, nodeName, nodePath, chanName)
				#ent.printInfo()
				lst.append(ent)
	if len(lst) > 0:
		lib = ExprLibExporter()
		lib.setLst(lst)
	return lib

def expExprLibFromHrc(outPath, rootPath="/obj/root"):
	lib = getExprLibFromHrc(rootPath)
	lib.save(outPath)

def getNodeExprLib(node, excl = None):
	if not node: return
	lst = []
	for parm in node.parms():
		try: expr = parm.expression()
		except: expr = None
		if expr and (not excl or not expr in excl):
			nodeName = node.name()
			nodePath = None
			fullPath = node.path()
			sep = fullPath.rfind("/")
			if sep >= 0: nodePath = fullPath[:sep]
			chanName = parm.name()
			ent = ExprEntry(expr, nodeName, nodePath, chanName)
			lst.append(ent)
	if len(lst) > 0:
		lib = ExprLibExporter()
		lib.setLst(lst)
	return lib
