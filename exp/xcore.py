# Author: Sergey Chaban <sergey.chaban@gmail.com>

import sys
import os
import re
import struct
from math import *
from array import array

try: xrange
except: xrange = range

def dbgmsg(msg):
	sys.stdout.write(str(msg) + "\n")

def align(x, y): return ((x + (y - 1)) & (~(y - 1)))

def sq(x): return x*x

def sinc(x):
	if abs(x) < 1.0e-4: return 1.0
	return sin(x) / x

def rcp0(x):
	if x: return 1.0 / x
	return 0.0

def div0(x, y):
	if y: return x / y
	return 0.0

def clz32(x):
	if x == 0: return 32
	n = 0
	s = 16
	while s:
		m = ((1 << s) - 1) << (32 - s)
		if (x & m) == 0:
			n += s
			x <<= s
		s >>= 1
	return n

def ctz32(x):
	if x == 0: return 32
	n = 0
	s = 16
	while s:
		m = (1 << s) - 1
		if (x & m) == 0:
			n += s
			x >>= s
		s >>= 1
	return n

def bitLen32(x): return 32 - clz32(x)

def popCnt(x):
	n = 0
	while x:
		x &= x-1
		n += 1
	return n

def getBitsF32(x):
	return struct.unpack("I", struct.pack("f", x))[0]

def setBitsF32(x):
	a = array('f')
	s = chr(x&0xFF) + chr((x>>8)&0xFF) + chr((x>>16)&0xFF) + chr((x>>24)&0xFF)
	a.fromstring(s)
	return a[0]

def getFractionBitsF32(x):
	b = getBitsF32(x)
	if (b & 0x7FFFFFFF) == 0: return 0
	return b & ((1<<23)-1)

def getExponentBitsF32(x):
	b = getBitsF32(x)
	return (b>>23)&0xFF

def getFractionF32(x):
	if x == 0.0: return 0
	m = getFractionBitsF32(x)
	return 1.0 + float(m) / float(1<<23)

def getExponentF32(x):
	return getExponentBitsF32(x) - 127

def buildF32(e, m, s=0):
	return setBitsF32((((int(e)+127)&0xFF) << 23) | int(m) | ((int(s)&1)<<31))

def halfFromBits(b):
	s = (b >> 16) & (1 << 15)
	b &= 0x7FFFFFFF
	if b > 0x477FE000: return 0x7C00 | s
	if b < 0x38800000:
		r = 0x70 + 1 - (b >> 23)
		t = 1 << 23
		b &= t - 1
		b |= t
		b >>= r
	else:
		b += 0xC8000000
	a = (b >> 13) & 1
	b += (1 << 12) - 1
	b += a
	b >>= 13
	b &= (1 << 15) - 1
	return b | s

def limF32(x):
	a = array('f', [x])
	s = a.tostring()
	b = array('f')
	b.fromstring(s)
	return b[0]

def vecLimF32(v):
	return [limF32(v[i]) for i in xrange(len(v))]

def absFracF32(x):
	return abs(limF32(x) - int(x))

def f16(x):
	if x == 0: return 0
	return halfFromBits(getBitsF32(x))

def i16(x, scl = 0x7FFF):
	return max(min(int(round(x * scl)), 32767), -32768)

def u16(x, scl = 0xFFFF):
	if isinf(x) or isnan(x): return 0
	return max(min(int(round(x * scl)), 0xFFFF), 0)

def ceilDiv(x, div):
	res = x // div
	if x % div: res += 1
	return res

def vecZero():
	return [0.0 for i in xrange(3)]

def vecMag2(v):
	l = 0.0
	for i in xrange(len(v)):
		l += v[i]*v[i]
	return l

def vecMag(v): return sqrt(vecMag2(v))

def vecScl(v, s): return [v[i] * s for i in xrange(len(v))]

def vecDot(v1, v2):
	d = 0.0
	n = min(len(v1), len(v2))
	for i in xrange(n): d += v1[i]*v2[i]
	return d

def vecAdd(v1, v2):
	n = min(len(v1), len(v2))
	return [v1[i] + v2[i] for i in xrange(n)]

def vecSub(v1, v2):
	n = min(len(v1), len(v2))
	return [v1[i] - v2[i] for i in xrange(n)]

def vecMul(v1, v2):
	n = min(len(v1), len(v2))
	return [v1[i] * v2[i] for i in xrange(n)]

def vecDist2(v1, v2): return vecMag2(vecSub(v1, v2))

def vecDist(v1, v2): return vecMag(vecSub(v1, v2))

def mkUnitVec(x, y, z):
	mag = sqrt(sq(x) + sq(y) + sq(z))
	if mag:
		d = 1.0 / mag
		x *= d
		y *= d
		z *= d
	return [x, y, z]

def mkUnitQuat(x, y, z, w):
	mag = sqrt(sq(x) + sq(y) + sq(z) + sq(w))
	if mag:
		d = 1.0 / mag
		x *= d
		y *= d
		z *= d
		w *= d
	return [x, y, z, w]


def encodeUnitU8(x):
	x = min(max(x, 0.0), 1.0)
	return int(round(x * 255.0))

def decodeUnitU8(x):
	x = min(max(x, 0), 255)
	return x / 255.0

def encodeSgnUnitU8(x):
	x = min(max(x, -1.0), 1.0)
	x = (x + 1.0) / 2.0
	return encodeUnitU8(x)

def decodeSgnUnitU8(x):
	return decodeUnitU8(x)*2.0 - 1.0

def encodeSgnUnitI16(x):
	x = min(max(x, -1.0), 1.0)
	return int(round(x * 32767.0))

def decodeSgnUnitI16(x):
	x = min(max(x, -32767), 32767)
	return x / 32767.0

def encodeVecI16(v):
	return [encodeSgnUnitI16(v[i]) for i in xrange(len(v))]

def decodeVecI16(v):
	return [decodeSgnUnitI16(v[i]) for i in xrange(len(v))]

def signNZ(x):
	if x >= 0.0: return 1.0
	return -1.0

# http://lgdv.cs.fau.de/publications/publication/Pub.2010.tech.IMMD.IMMD9.onfloa/
# http://jcgt.org/published/0003/02/01/
def encodeOcta(v):
	x = v[0]
	y = v[1]
	z = v[2]
	ax = abs(x)
	ay = abs(y)
	az = abs(z)
	d = rcp0(ax + ay + az)
	ox = x * d
	oy = y * d
	if z < 0.0:
		tx = (1.0 - abs(oy)) * signNZ(ox)
		ty = (1.0 - abs(ox)) * signNZ(oy)
		ox = tx
		oy = ty
	return [ox, oy]

def decodeOcta(oct):
	x = oct[0]
	y = oct[1]
	ax = abs(x)
	ay = abs(y)
	z = 1.0 - ax - ay
	if z < 0.0:
		x = (1.0 - ay) * signNZ(x)
		y = (1.0 - ax) * signNZ(y)
	return mkUnitVec(x, y, z)

def encodeOctaI16(v):
	return encodeVecI16(encodeOcta(v))

def decodeOctaI16(v):
	return decodeOcta(decodeVecI16(v))

def encodeClrMax(c):
	cc = [max(c[i], 0.0) for i in xrange(3)]
	s = 1.0/max(max(max(cc[0], cc[1]), cc[2]), 1.0)
	return [cc[0]*s, cc[1]*s, cc[2]*s, s]

def decodeVec(v, nbits = 16, nrm = True):
	n = len(v)
	s = 1.0 / float((1<<(nbits-1))-1)
	res = [v[i]*s for i in xrange(n)]
	if nrm:
		sqmag = 0.0
		for x in res: sqmag += x*x
		if sqmag:
			imag = 1.0 / sqrt(sqmag)
			for i in xrange(len(res)):
				res[i] *= imag
	return res

def encodeVec(v, nbits = 16, fdiff = None, tol = 1.0e-8, nrm = True, retTag = None):
	n = len(v)
	s = float((1<<(nbits-1))-1)
	sv = [v[i] * s for i in xrange(n)]
	if not fdiff:
		if retTag: retTag[0] = -1
		return [int(round(sv[i])) for i in xrange(n)]
	enc = [0 for i in xrange(n)]
	tmp = [0 for i in xrange(n)]
	cnt = 1 << (2*n)
	minDiff = 1.0e10
	minTag = -1
	for m in xrange(cnt):
		for i in xrange(n):
			mode = (m >> (i*2)) & 3
			val = 0
			if mode == 0:
				val = round(sv[i])
			elif mode == 1:
				val = sv[i]
			elif mode == 2:
				val = ceil(sv[i])
			elif mode == 3:
				val = floor(sv[i])
			tmp[i] = int(val)
		dv = decodeVec(tmp, nbits, nrm)
		diff = fdiff(v, dv)
		if diff < minDiff:
			minTag = m
			minDiff = diff
			for i in xrange(n): enc[i] = tmp[i]
		if diff <= tol: break
	if retTag:
		retTag[0] = minTag
	return enc

def encodeVecTrunc(v, nbits = 16):
	n = len(v)
	s = float((1<<(nbits-1))-1)
	sv = [v[i] * s for i in xrange(n)]
	return [int(sv[i]) for i in xrange(n)]

def encodeQuat(q, nbits = 16, tol = 1.0e-8, retTag = None):
	return encodeVec(q, nbits, quatDiffEstimate, tol, True, retTag)

def quatDiffEstimate(a, b):
	n = min(len(a), len(b))
	d = 0
	for i in xrange(n): d += a[i]*b[i]
	return 1.0 - min(1.0, abs(d))

def quatGetSV(q, angNrm = True):
	v = mkUnitVec(q[0], q[1], q[2])
	w = min(max(q[3], -1.0), 1.0)
	a = acos(w)
	if angNrm: a /= pi
	for i in xrange(3): v[i] *= a
	return v

def quatFromSV(v, angNrm = True):
	(x, y, z) = v
	a = sqrt(sq(x) + sq(y) + sq(z))
	if angNrm:
		x *= pi
		y *= pi
		z *= pi
		a *= pi
	w = cos(a)
	s = sinc(a)
	q = mkUnitQuat(x*s, y*s, z*s, w)
	return q

def packTupF32(t):
	s = b""
	for x in t: s += struct.pack("f", x)
	return s

# http://www.isthe.com/chongo/tech/comp/fnv/index.html
def strHash32(s):
	h = 2166136261
	for c in s:
		h *= 16777619
		h ^= ord(c)
		h &= 0xFFFFFFFF
	return h

def strHash16(s):
	h = strHash32(s)
	h = (h >> 16) ^ (h & 0xFFFF)
	return h & 0xFFFF

def fresnelFromIOR(ior):
	r = (ior - 1.0) / (ior + 1.0)
	return r * r

def rgba8(data, gamma = 1.0):
	n = len(data) // 4
	fmt = str(n) + "f"
	lst = list(struct.unpack(fmt, data))
	if gamma > 0.0 and gamma != 1.0:
		igamma = 1.0 / gamma
		igv = [igamma, igamma, igamma, 1.0]
		res = [int(round((min(max(lst[i], 0.0), 1.0) ** igv[i % 4]) * 255.0)) for i in xrange(n)]
	else:
		res = [int(round(min(max(lst[i], 0.0), 1.0) * 255.0)) for i in xrange(n)]
	return array("B", res).tostring()


class XORDER:
	def __init__(self): pass
XORDER.SRT = 0
XORDER.STR = 1
XORDER.RST = 2
XORDER.RTS = 3
XORDER.TSR = 4
XORDER.TRS = 5

def xformOrdFromStr(str):
	try: return getattr(XORDER, str.upper())
	except: return XORDER.SRT


class RORDER:
	def __init__(self): pass
RORDER.XYZ = 0
RORDER.XZY = 1
RORDER.YXZ = 2
RORDER.YZX = 3
RORDER.ZXY = 4
RORDER.ZYX = 5

def rotOrdFromStr(str):
	try: return getattr(RORDER, str.upper())
	except: return RORDER.XYZ


class BinWriter:
	def __init__(self): pass
	
	def open(self, name):
		self.name = name
		self.file = open(name, "wb")

	def close(self): self.file.close()
	def seek(self, offs): self.file.seek(offs)
	def getFile(self): return self.file
	def getPos(self): return self.file.tell()
	def writeI8(self, val): array('b', [val]).tofile(self.file)
	def writeU8(self, val): array('B', [val]).tofile(self.file)
	def writeI32(self, val): array('i', [val]).tofile(self.file)
	def writeU32(self, val): array('I', [val]).tofile(self.file)
	def writeI16(self, val): array('h', [val]).tofile(self.file)
	def writeU16(self, val): array('H', [val]).tofile(self.file)
	def writeF32(self, val): array('f', [val]).tofile(self.file)
	def writeF16(self, val):
		if val == 0:
			self.writeI16(0)
			return
		bits = getBitsF32(val)
		self.writeU16(halfFromBits(bits))
	def writeFV(self, v):
		for x in v: self.writeF32(x)
	def writeFV16(self, v):
		for x in v: self.writeF16(x)
	def writeFQ(self, v):
		n = len(v)
		if n > 4: n = 4
		for i in xrange(n): self.writeF32(v[i])
		for i in xrange(4-n): self.writeF32(0)
	def writeRGB1(self, v):
		n = len(v)
		if n > 3: n = 3
		for i in xrange(n): self.writeF32(v[i])
		for i in xrange(4-n): self.writeF32(1.0)
	def writeFOURCC(self, str):
		n = len(str)
		if n > 4: n = 4
		for i in xrange(n): self.writeU8(ord(str[i]))
		for i in xrange(4-n): self.writeU8(0)
	def writeStr(self, str):
		for c in str: self.writeU8(ord(c))
		self.writeU8(0)
	def writeBytes(self, data, nbytes):
		for i in xrange(nbytes):
			self.writeU8(data & 0xFF)
			data >>= 8

	def writeIV16(self, v):
		for x in v: self.writeI16(x)

	def writeU24(self, x):
		self.writeU16(x & 0xFFFF)
		self.writeU8((x>>16) & 0xFF)

	def patch(self, offs, val):
		nowPos = self.getPos()
		self.seek(offs)
		self.writeU32(val)
		self.seek(nowPos)

	def patchCur(self, offs): self.patch(offs, self.getPos())

	def align(self, x):
		pos0 = self.getPos()
		pos1 = align(pos0, x)
		for i in xrange(pos1-pos0): self.writeU8(0xFF)

class BinReader:
	def __init__(self): pass
	
	def open(self, name):
		self.name = name
		self.file = open(name, "rb")

	def close(self): self.file.close()
	def seek(self, offs): self.file.seek(offs)
	def getPos(self): return self.file.tell()
	def readI8(self):
		data = array('b')
		data.fromfile(self.file, 1)
		return data[0]
	def readU8(self):
		data = array('B')
		data.fromfile(self.file, 1)
		return data[0]
	def readI16(self):
		data = array('h')
		data.fromfile(self.file, 1)
		return data[0]
	def readU16(self):
		data = array('H')
		data.fromfile(self.file, 1)
		return data[0]
	def readI32(self):
		data = array('i')
		data.fromfile(self.file, 1)
		return data[0]
	def readU32(self):
		data = array('I')
		data.fromfile(self.file, 1)
		return data[0]
	def readF32(self):
		data = array('f')
		data.fromfile(self.file, 1)
		return data[0]
	def readStringLim(self, r = xrange(0x80)):
		data = array("B")
		for i in r:
			data.fromfile(self.file, 1)
			if not data[i]:
				data.pop()
				break
		return data.tostring()

class StrList:
	def __init__(self, tsort = 32):
		self.lst = []
		self.map = {}
		self.count = 0
		self.tsort = tsort
		self.sortedLst = None
		self.sortedMap = None

	def add(self, str):
		if str not in self.map:
			self.map[str] = len(self.lst)
			self.lst.append(str)
			self.count += len(str) + 1
		return self.map[str]

	def get(self, idx):
		if idx < 0 or idx >= len(self.lst): return None
		return self.lst[idx]

	def getIdx(self, str):
		if str in self.map: return self.map[str]
		return -1

	def write(self, bw):
		strTop = bw.getPos()
		strNum = len(self.lst)
		dataSize = self.count
		dataSize += 8 # header: data_size; str_num;
		dataSize += 4*strNum # offsets
		dataSize += 2*strNum # hashes
		if self.sortedLst: dataSize += 1
		bw.writeU32(dataSize)
		bw.writeU32(strNum)
		offsTop = bw.getPos()
		for i in xrange(strNum): bw.writeU32(0)
		if self.sortedLst:
			for s in self.sortedLst: bw.writeU16(s[0])
			for i, s in enumerate(self.sortedLst):
				bw.patch(offsTop + i*4, bw.getPos() - strTop)
				bw.writeStr(s[1])
		else:
			for s in self.lst: bw.writeU16(strHash16(s))
			for i, s in enumerate(self.lst):
				bw.patch(offsTop + i*4, bw.getPos() - strTop)
				bw.writeStr(s)
		if self.sortedLst: bw.writeU8(0x81)

	def addNameAndPath(self, fullPath):
		name = fullPath
		sep = fullPath.rfind("/")
		if sep >= 0: name = fullPath[sep+1:]
		nameId = self.add(name)
		pathId = -1
		if sep >= 0: pathId = self.add(fullPath[:sep])
		return nameId, pathId

	def finalize(self):
		strNum = len(self.lst)
		if self.tsort > 0 and strNum >= self.tsort:
			self.sortedLst = []
			for i in xrange(strNum):
				s = self.lst[i]
				h = strHash16(s)
				self.sortedLst.append([h, s, i])
			self.sortedLst.sort(key=lambda s: s[0])
			self.sortedMap = [-1 for i in xrange(strNum)]
			for i in xrange(strNum):
				self.sortedMap[self.sortedLst[i][2]] = i

	def getWriteId(self, idx):
		if self.sortedMap:
			if idx >= 0 and idx < len(self.sortedMap):
				return self.sortedMap[idx]
		return idx


class VecList:
	def __init__(self):
		self.lst = []
		self.map = {}
		self.idx = []
		self.ptr = 0

	def add(self, vec):
		v = tuple(vec)
		lv = len(v)
		if lv < 1: return -1
		if v not in self.map:
			id = len(self.lst)
			self.map[v] = id
			if (lv == 4):
				self.map[(v[0], v[1], v[2])] = id
				self.map[(v[0], v[1])] = id
			elif (lv == 3):
				self.map[(v[0], v[1])] = id
			self.lst.append(v)
			self.idx.append(self.ptr | (((lv-1)&3)<<30))
			self.ptr += lv
		return self.map[v]

	def write(self, bw):
		vecNum = len(self.lst)
		dataSize = 8 + len(self.idx)*4 + self.ptr*4
		bw.writeU32(dataSize) # data_size
		bw.writeU32(vecNum)
		for i in self.idx: bw.writeU32(i)
		for v in self.lst: bw.writeFV(v)

	def num(self):
		return len(self.lst)


class ExprStrings:
	def __init__(self):
		self.lst = []
		self.map = {} # str -> ptr
		self.tbl = {} # ptr -> idx
		self.ptr = 0

	def add(self, str):
		if str not in self.map:
			idx = len(self.lst)
			self.map[str] = idx
			self.tbl[self.ptr] = idx
			self.lst.append(str)
			self.ptr += len(str) + 1
		return self.map[str]

	def ptrToIdx(self, ptr):
		if ptr in self.tbl: return self.tbl[ptr]
		return -1

	def getByIdx(self, idx):
		if idx < 0 or idx >= len(self.lst): return None
		return self.lst[idx]

	def getByPtr(self, ptr):
		idx = self.ptrToIdx(ptr)
		return self.getByIdx(idx)

	def get(self, idx): return self.getByIdx(idx)

	def getPtr(self, str):
		if str in self.map: return self.map[str]
		return -1

	def getCount(self): return len(self.lst)

	def write(self, bw):
		tblTop = bw.getPos()
		for i, s in enumerate(self.lst):
			bw.writeU32(strHash32(s)) # +00
			bw.writeU16(0) # +04 offs
			bw.writeU16(len(s)) # +06
		top = bw.getPos()
		for i, s in enumerate(self.lst):
			now = bw.getPos()
			bw.seek(tblTop + i*8 + 4)
			bw.writeU16(now - top)
			bw.seek(now)
			bw.writeStr(s)

class ExprVals:
	def __init__(self):
		self.lst = []
		self.map = {}

	def add(self, val):
		if val in self.map:
			return self.map[val]
		id = len(self.lst)
		self.map[val] = id
		self.lst.append(val)
		return id

	def get(self, idx):
		if idx < 0 or idx >= len(self.lst): return 0.0
		return self.lst[idx]

	def getIdx(self, val):
		if val in self.map: return self.map[val]
		return -1

	def getCount(self): return len(self.lst)

	def write(self, bw):
		for val in self.lst: bw.writeF32(val)


class ExprCode:
	def __init__(self):
		self.op = ExprCode.NOP
		self.prio = -1
		self.info = -1

	def mkVar(self, symIdx):
		self.op = ExprCode.VAR
		self.prio = 0
		self.info = int(symIdx)

	def mkNum(self, numIdx):
		self.op = ExprCode.NUM
		self.prio = 0
		self.info = int(numIdx)

	def mkStr(self, strIdx):
		self.op = ExprCode.STR
		self.prio = 0
		self.info = int(strIdx)

	def mkFun(self, funId):
		self.op = ExprCode.FUN
		self.prio = 0
		self.info = int(funId)

	def mkEnd(self):
		self.op = ExprCode.END
		self.prio = 0
		self.info = -1

	def getOpName(self):
		names = ["NOP", "END", "NUM", "STR", "VAR", "CMP", "ADD", "SUB", "MUL", "DIV", "MOD", "NEG", "FUN", "XOR", "AND", "OR"]
		idx = self.op
		if idx >= len(names): return "<bad>"
		opname = names[idx]
		if self.op == ExprCode.CMP:
			cmps = ["EQ", "NE", "LT", "LE", "GT", "GE"]
			cmpOp = "<bad>"
			idx = self.info
			if idx < len(cmps): cmpOp = cmps[idx]
			opname += "." + cmpOp
		elif self.op == ExprCode.FUN:
			funName = "<bad>"
			idx = self.info
			if idx < len(Expr.funcList):
				funName = Expr.funcList[idx]
			opname += " " + funName
		return opname

	def write(self, bw):
		bw.writeU8(self.op)
		bw.writeI8(self.prio)
		bw.writeI16(self.info)

ExprCode.NOP = 0
ExprCode.END = 1
ExprCode.NUM = 2
ExprCode.STR = 3
ExprCode.VAR = 4
ExprCode.CMP = 5
ExprCode.ADD = 6
ExprCode.SUB = 7
ExprCode.MUL = 8
ExprCode.DIV = 9
ExprCode.MOD = 10
ExprCode.NEG = 11
ExprCode.FUN = 12
ExprCode.XOR = 13
ExprCode.AND = 14
ExprCode.OR  = 15

ExprCode.CMP_EQ = 0
ExprCode.CMP_NE = 1
ExprCode.CMP_LT = 2
ExprCode.CMP_LE = 3
ExprCode.CMP_GT = 4
ExprCode.CMP_GE = 5

class ExprToken:
	def __init__(self):
		self.reset()

	def reset(self):
		self.type = ExprToken.TYPE_BAD
		self.opc = ExprCode()
		self.sym = ""
		self.sep = ''
		self.num = 0.0

	def isBad(self): return self.type == ExprToken.TYPE_BAD
	def isSym(self): return self.type == ExprToken.TYPE_SYM
	def isNum(self): return self.type == ExprToken.TYPE_NUM
	def isStr(self): return self.type == ExprToken.TYPE_STR
	def isSep(self): return self.type == ExprToken.TYPE_SEP
	def isOp(self): return self.type == ExprToken.TYPE_OP

ExprToken.TYPE_BAD = 0
ExprToken.TYPE_SYM = 1
ExprToken.TYPE_NUM = 2
ExprToken.TYPE_STR = 3
ExprToken.TYPE_SEP = 4
ExprToken.TYPE_OP  = 5

class Expr:
	def __init__(self):
		self.reset()

	def reset(self):
		self.text = ""
		self.ptr = 0
		self.ch = '\0'
		self.tok = ExprToken()
		self.symStk = []
		self.opStk = []
		self.code = []
		self.vals = ExprVals()
		self.strs = ExprStrings()

	def isEOF(self): return self.ch == '\0'
	def isSpace(self): return self.ch.isspace()
	def isIdStart(self): return self.ch.isalpha() or self.ch == '_' or self.ch == '$' or self.ch == '@'
	def isIdChar(self): return self.ch.isalnum() or self.ch == '_'
	def isSep(self): return self.ch == ')' or self.ch == ','
	def isDigit(self): return self.ch.isdigit()
	def isNum(self): return self.ch.isdigit() or self.ch == '.'
	def isStrStart(self): return self.ch == '"'

	def negCk(self):
		self.negFlg = False
		if self.ch == '-':
			if self.oldTok.isBad() or self.oldTok.isSep() or self.oldTok.isOp():
				self.skipSpace()
				self.read()
				self.negFlg = True
		return self.negFlg

	def read(self):
		if self.text and self.ptr < len(self.text):
			self.ch = self.text[self.ptr]
			self.ptr += 1
		else:
			self.ch = '\0'

	def unread(self):
		if self.text and self.ptr > 0:
			self.ptr -= 1
			self.ch = self.text[self.ptr]

	def skipSpace(self):
		while self.isSpace(): self.read()

	def pushSym(self):
		sym = self.ch
		self.read()
		while self.isIdChar():
			sym += self.ch
			self.read()
		self.symStk.append(sym)

	def getTopSym(self):
		tos = len(self.symStk) - 1
		if tos < 0: return None
		return self.symStk[tos]

	def popSym(self):
		tos = len(self.symStk) - 1
		if tos < 0: return None
		sym = self.symStk.pop()
		return sym

	def readNum(self):
		s = self.ch
		self.read()
		while self.isNum():
			s += self.ch
			self.read()
		return float(s)

	def getCode(self):
		code = ExprCode()
		ch = self.ch
		if ch == '(':
			code.op = ExprCode.NOP
			code.prio = 0
		elif ch == '<' or ch == '>' or ch == '=' or ch == '!':
			code.op = ExprCode.CMP
			code.prio = 1
		elif ch == '^':
			code.op = ExprCode.XOR
			code.prio = 2
		elif ch == '&':
			code.op = ExprCode.AND
			code.prio = 2
		elif ch == '|':
			code.op = ExprCode.OR
			code.prio = 2
		elif ch == '+':
			code.op = ExprCode.ADD
			code.prio = 3
		elif ch == '-':
			code.op = ExprCode.SUB
			code.prio = 3
		elif ch == '*':
			code.op = ExprCode.MUL
			code.prio = 4
		elif ch == '/':
			code.op = ExprCode.DIV
			code.prio = 4
		elif ch == '%':
			code.op = ExprCode.MOD
			code.prio = 4

		if code.op == ExprCode.CMP:
			self.read()
			if self.ch != '=':
				self.unread()
				if ch == '<': code.info = ExprCode.CMP_LT
				elif ch == '>': code.info = ExprCode.CMP_GT
				else: dbgmsg("Invalid CMP")
			else:
				if ch == '<': code.info = ExprCode.CMP_LE
				elif ch == '>': code.info = ExprCode.CMP_GE
				elif ch == '=': code.info = ExprCode.CMP_EQ
				elif ch == '!': code.info = ExprCode.CMP_NE
				else: dbgmsg("Invalid CMP")
		return code

	def getTok(self):
		self.oldTok = self.tok
		self.tok = ExprToken()
		self.skipSpace()
		if self.isIdStart():
			self.pushSym()
			self.tok.type = ExprToken.TYPE_SYM
			self.tok.sym = self.getTopSym()
		elif self.negCk():
			code = ExprCode()
			code.op = ExprCode.NEG
			code.prio = 5
			self.tok.type = ExprToken.TYPE_OP
			self.tok.opc = code
		elif self.isNum():
			num = self.readNum()
			if 1 and self.isOpTopNeg():
				num = -num
				self.opStk.pop()
			self.tok.type = ExprToken.TYPE_NUM
			self.tok.num = num
		elif self.isStrStart():
			str = ""
			endCh = self.ch
			while not self.isEOF():
				self.read()
				if self.ch == endCh: break
				str += self.ch
			self.read() # final "
			self.tok.type = ExprToken.TYPE_STR
			self.tok.sym = str
		elif self.isSep():
			self.tok.type = ExprToken.TYPE_SEP
			self.tok.sep = self.ch
			self.read()
		else:
			code = self.getCode()
			if code.prio < 0:
				self.tok.type = ExprToken.TYPE_BAD
			else:
				self.tok.type = ExprToken.TYPE_OP
				self.tok.opc = code
			self.read()

	def emitEnd(self):
		code = ExprCode()
		code.mkEnd()
		self.code.append(code)

	def emitVar(self, sym):
		symIdx = self.strs.add(sym)
		code = ExprCode()
		code.mkVar(symIdx)
		self.code.append(code)

	def emitNum(self, num):
		numIdx = self.vals.add(num)
		code = ExprCode()
		code.mkNum(numIdx)
		self.code.append(code)

	def emitStr(self, str):
		strIdx = self.strs.add(str)
		code = ExprCode()
		code.mkStr(strIdx)
		self.code.append(code)

	def ckOpTop(self):
		tos = len(self.opStk) - 1
		if tos < 0: return False
		if self.opStk[tos].op == ExprCode.NOP: return False # sentinel
		return True

	def isOpTopFun(self):
		tos = len(self.opStk) - 1
		if tos < 0: return False
		if self.opStk[tos].op == ExprCode.FUN: return True
		return False

	def isOpTopNeg(self):
		tos = len(self.opStk) - 1
		if tos < 0: return False
		if self.opStk[tos].op == ExprCode.NEG: return True
		return False

	def getOpTopPrio(self):
		tos = len(self.opStk) - 1
		if tos < 0: return -1
		return self.opStk[tos].prio

	def emitOpTop(self):
		if self.ckOpTop():
			code = self.opStk.pop()
			self.code.append(code)

	def compile(self, text):
		self.reset()
		self.text = text
		self.negFlg = False

		while True:
			self.getTok()
			if self.tok.isSym():
				sym = self.tok.sym
				if sym in Expr.funcList:
					funOp = ExprCode()
					funId = Expr.funcList.index(sym)
					funOp.mkFun(funId)
					self.opStk.append(funOp)
				else:
					self.emitVar(sym)
			elif self.tok.isNum():
				self.emitNum(self.tok.num)
			elif self.tok.isStr():
				self.emitStr(self.tok.sym)
			if self.tok.isSep():
				if self.tok.sep == ')':
					while self.ckOpTop(): self.emitOpTop()
					self.opStk.pop() # pop sentinel
					if self.isOpTopFun():
						self.emitOpTop()
				elif self.tok.sep == ',':
					while self.ckOpTop(): self.emitOpTop()
			if self.tok.isOp():
				if self.tok.opc.prio > 0:
					while len(self.opStk) > 0:
						if self.getOpTopPrio() < self.tok.opc.prio: break
						self.emitOpTop()
				self.opStk.append(self.tok.opc)
			if self.isEOF(): break

		while len(self.opStk) > 0:
			self.emitOpTop()
		self.emitEnd()

	def disasm(self):
		for addr, code in enumerate(self.code):
			opName = code.getOpName()
			opStr = opName
			if code.op == ExprCode.NUM:
				numIdx = code.info
				val = self.vals.get(numIdx)
				opStr += " " + str(val)
			elif code.op == ExprCode.STR:
				strIdx = code.info;
				s = self.strs.get(strIdx)
				opStr += ' "' + s + '"'
			elif code.op == ExprCode.VAR:
				symIdx = code.info
				sym = self.strs.get(symIdx)
				opStr += " " + sym
			dbgmsg('%(#)02X' % {"#":addr} + " : " + opStr)

	def write(self, bw):
		top = bw.getPos()
		bw.writeFOURCC("CEXP") # +00 sig
		bw.writeU32(0) # +04 len
		ncode = len(self.code)
		nvals = self.vals.getCount()
		nstrs = self.strs.getCount()
		bw.writeI32(nvals) # +08
		bw.writeI32(ncode) # +0C
		bw.writeI32(nstrs) # +10
		self.vals.write(bw)
		for code in self.code: code.write(bw)
		self.strs.write(bw)
		bw.patch(top + 4, bw.getPos() - top) # len

Expr.funcList = [
	"abs", "acos", "asin", "atan", "atan2", "ceil",
	"ch",
	"clamp", "cos", "deg", "detail", "distance", "exp",
	"fit", "fit01", "fit10", "fit11",
	"floor", "frac", "if", "int", "length",
	"log", "log10", "max", "min", "pow", "rad",
	"rint", "round", "sign", "sin", "sqrt", "tan"
]


class BaseExporter:
	def __init__(self):
		self.sig = "XDAT"
		self.strLst = StrList()
		self.flags = 0
		self.nameId = -1
		self.pathId = -1

	def writeHead(self, bw, top): pass
	def writeData(self, bw, top): pass

	def writeStrId16(self, bw, idx):
		bw.writeI16(self.strLst.getWriteId(idx))

	def writeStrId32(self, bw, idx):
		bw.writeI32(self.strLst.getWriteId(idx))

	def write(self, bw):
		self.strLst.finalize()
		top = bw.getPos()
		bw.writeFOURCC(self.sig) # +00 sig
		bw.writeU32(self.flags) # +04 flags
		bw.writeU32(0) # +08 fileSize
		bw.writeU32(0) # +0C headSize
		bw.writeU32(0) # +10 offsStr
		self.writeStrId16(bw, self.nameId) # +14 nameId
		self.writeStrId16(bw, self.pathId) # +16 pathId
		bw.writeU32(0) # +18 filePathLen (set by loader)
		bw.writeU32(0) # +1C offsExt
		self.writeHead(bw, top)
		bw.patch(top + 0xC, bw.getPos() - top) # headSize
		bw.align(0x10)
		bw.patch(top + 0x10, bw.getPos() - top) # offsStr
		self.strLst.write(bw)
		self.writeData(bw, top)
		bw.patch(top + 8, bw.getPos() - top) # fileSize

	def setNameFromPath(self, fullPath):
		name = fullPath
		sep = fullPath.rfind("/")
		if sep >= 0: name = fullPath[sep+1:]
		self.nameId = self.strLst.add(name)
		self.pathId = -1
		if sep >= 0: self.pathId = self.strLst.add(fullPath[:sep])

	def save(self, outPath):
		bw = BinWriter()
		bw.open(outPath)
		self.write(bw)
		bw.close()


class FileCatEntry:
	def __init__(self, nameId, fnameId):
		self.nameId = nameId
		self.fnameId = fnameId

class FileCatalogue(BaseExporter):
	def __init__(self):
		BaseExporter.__init__(self)
		self.sig = "FCAT"
		self.lst = []

	def setName(self, name):
		self.setNameFromPath(name)

	def addFile(self, name, fname):
		nameId  = self.strLst.add(name)
		fnameId = self.strLst.add(fname)
		self.lst.append(FileCatEntry(nameId, fnameId))

	def writeHead(self, bw, top):
		n = len(self.lst)
		bw.writeU32(n) # +20
		self.patchPos = bw.getPos()
		bw.writeU32(0) # -> entries[]

	def writeData(self, bw, top):
		bw.align(0x10)
		bw.patch(self.patchPos, bw.getPos() - top)
		for ent in self.lst:
			self.writeStrId32(bw, ent.nameId)
			self.writeStrId32(bw, ent.fnameId)
