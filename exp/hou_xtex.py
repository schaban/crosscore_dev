# Author: Sergey Chaban <sergey.chaban@gmail.com>

import sys
import hou
import os
import imp
import re
import array
import struct
import inspect
from math import *

import xcore
import xhou

try: xrange
except: xrange = range

class TexFormat:
	def __init__(self): pass
TexFormat.RGBA_BYTE_LINEAR = 0
TexFormat.RGBA_BYTE_GAMMA2 = 1

class TexExporter(xcore.BaseExporter):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		self.sig = "XTEX"

	def build(self, cop, fmt, expTexName = None):
		self.cop = cop
		if not cop: return
		if not expTexName: expTexName = cop.path()
		self.setNameFromPath(expTexName)
		self.w = cop.xRes()
		self.h = cop.yRes()
		self.fmt = fmt
		self.noMIP = False
		for prmName in ["tex_nomip", "txd_nomip"]:
			prm = cop.parm(prmName)
			if prm:
				if prm.evalAsInt():
					self.noMIP = True
					break
		self.noBias = False
		for prmName in ["tex_nobias", "txd_nobias"]:
			prm = cop.parm(prmName)
			if prm:
				if prm.evalAsInt():
					self.noBias = True
					break
		w = self.w
		h = self.h
		dfmt = hou.imageDepth.Float32
		esize = 4
		useStrs = sys.version_info[0] < 3
		if useStrs:
			c = cop.allPixelsAsString(plane="C", depth=dfmt)
		else:
			cdat = cop.allPixels(plane="C")
			c = struct.pack(str(w*h*3)+"f", *cdat)
		if "A" in cop.planes():
			if useStrs:
				a = cop.allPixelsAsString(plane="A", depth=dfmt)
			else:
				adat = cop.allPixels(plane="A")
				a = b""
				for i in xrange(w*h):
					a += struct.pack("f", adat[i])
		else:
			a = b""
			for i in xrange(w*h):
				a += struct.pack("f", 1.0)
		self.dataStrs = []
		for y in xrange(h):
			idx = (h - 1 - y) * w
			lc = c[idx*esize*3 : (idx + w) * esize*3]
			la = a[idx*esize : (idx + w) * esize]
			for x in xrange(w):
				rgba = lc[x*esize*3 : (x + 1) * esize*3]
				rgba += la[x*esize : (x + 1) * esize]
				if fmt == TexFormat.RGBA_BYTE_GAMMA2:
					self.dataStrs.append(xcore.rgba8(rgba, 2.0))
				elif fmt == TexFormat.RGBA_BYTE_LINEAR:
					self.dataStrs.append(xcore.rgba8(rgba, 1.0))

	def writeHead(self, bw, top):
		if not self.cop: return
		bw.writeU32(self.w) # +20
		bw.writeU32(self.h) # +24
		bw.writeU32(self.fmt) # +28
		bw.writeU32(0) # +2C reserved
		self.patchPos = bw.getPos()
		bw.writeU32(0) # +30 -> data
		bw.writeU32(0) # +34 -> cmpr
		bw.writeU32(0) # +38 reserved
		bw.writeU32(0) # +3C reserved
		for i in xrange(8): bw.writeU32(0) # +40 GPU wk [32 bytes]

	def writeData(self, bw, top):
		if not self.cop: return
		bw.align(0x10)
		bw.patch(self.patchPos + 0, bw.getPos() - top) # -> data
		f = bw.getFile()
		for s in self.dataStrs:
			f.write(s)

	def save(self, outPath):
		if not self.cop: return
		if self.noMIP: self.flags |= 1
		if self.noBias: self.flags |= 2
		xcore.BaseExporter.save(self, outPath)
