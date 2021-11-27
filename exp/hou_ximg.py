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

def writeBits(bw, bits, nbits):
	nbytes = xcore.ceilDiv(nbits, 8)
	wk = bits
	for i in xrange(nbytes):
		bw.writeU8(wk & 0xFF)
		wk >>= 8

class ImgPlane:
	def __init__(self, ximg, name, rawFlg = not True):
		self.ximg = ximg
		self.name = name
		self.nameId = ximg.strLst.add(name)
		if name == "a":
			self.data = ximg.cop.allPixels("A")
		else:
			self.data = ximg.cop.allPixels("C", xhou.getRGBComponentName(ximg.cop, name))
		ref = self.data[0]
		self.constFlg = True
		for val in self.data:
			if val != ref:
				self.constFlg = False
				break
		self.compress(rawFlg)

	def compress(self, rawFlg):
		self.minVal = min(self.data)
		self.maxVal = max(self.data)
		self.valOffs = self.minVal
		if self.valOffs > 0: self.valOffs = 0
		self.bitCnt = 0
		self.bits = 0
		self.minTZ = 32
		if self.constFlg:
			self.format = 0
			return
		if rawFlg:
			self.format = -1
			return
		self.format = 1
		for fval in self.data:
			fval -= self.valOffs
			ival = xcore.getBitsF32(fval) & ((1<<31)-1)
			self.minTZ = min(self.minTZ, xcore.ctz32(ival))
		tblSize = 1 << 8
		tbl = [0 for i in xrange(tblSize)]
		pred = 0
		hash = 0
		nlenBits = 5
		w = self.ximg.w
		h = self.ximg.h
		for y in xrange(h):
			for x in xrange(w):
				idx = (h-1-y)*w + x
				fval = self.data[idx] - self.valOffs
				ival = xcore.getBitsF32(fval) & ((1<<31)-1)
				ival >>= self.minTZ
				xor = ival ^ pred
				tbl[hash] = ival
				hash = ival >> 21
				hash &= tblSize - 1
				pred = tbl[hash]
				xlen = 0
				if xor: xlen = xcore.bitLen32(xor)
				dat = xlen
				if xlen: dat |= (xor & ((1<<xlen)-1)) << nlenBits
				self.bits |= dat << self.bitCnt
				self.bitCnt += nlenBits + xlen

	def writeInfo(self, bw):
		bw.writeU32(0) # +00 -> data
		self.ximg.writeStrId16(bw, self.nameId) # +04
		bw.writeU8(self.minTZ) # +06
		bw.writeI8(self.format) # +07 
		bw.writeF32(self.minVal) # +08
		bw.writeF32(self.maxVal) # +0C
		bw.writeF32(self.valOffs) # +10
		bw.writeU32(self.bitCnt) # +14
		bw.writeU32(0) # +18 reserved0
		bw.writeU32(0) # +1C reserved1

	def writeData(self, bw):
		if self.format == 0:
			bw.writeF32(self.data[0])
		elif self.format == 1:
			writeBits(bw, self.bits, self.bitCnt)
		else:
			w = self.ximg.w
			h = self.ximg.h
			for y in xrange(h):
				for x in xrange(w):
					idx = (h-1-y)*w + x
					bw.writeF32(self.data[idx])

class ImgExporter(xcore.BaseExporter):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		self.sig = "XIMG"

	def build(self, copPath, rawFlg = True):
		self.copPath = copPath
		self.nameId, self.pathId = self.strLst.addNameAndPath(copPath)
		self.cop = hou.node(copPath)
		self.w = self.cop.xRes()
		self.h = self.cop.yRes()
		self.planes = {}
		self.addPlane("r", rawFlg)
		self.addPlane("g", rawFlg)
		self.addPlane("b", rawFlg)
		self.addPlane("a", rawFlg)

	def addPlane(self, name, rawFlg = True):
		self.planes[name] = ImgPlane(self, name, rawFlg)

	def writeHead(self, bw, top):
		npln = len(self.planes)
		bw.writeU32(self.w) # +20
		bw.writeU32(self.h) # +24
		bw.writeU32(npln) # +28
		self.patchPos = bw.getPos()
		bw.writeI32(0) # +2C -> info

	def writeData(self, bw, top):
		plnLst = []
		for plnName in self.planes: plnLst.append(self.planes[plnName])
		npln = len(plnLst)
		bw.align(0x10)
		infoTop = bw.getPos()
		bw.patch(self.patchPos, bw.getPos() - top) # -> info
		for i in xrange(npln):
			plnLst[i].writeInfo(bw)
		for i, pln in enumerate(plnLst):
			bw.align(4)
			bw.patch(infoTop + (i*0x20), bw.getPos() - top)
			xcore.dbgmsg("Saving plane " + pln.name)
			pln.writeData(bw)

	def save(self, outPath):
		xcore.BaseExporter.save(self, outPath)


