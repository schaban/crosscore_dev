# Author: Sergey Chaban <sergey.chaban@gmail.com>

import sys
import os
import re
import random
import inspect
import importlib as imp
from math import *
from decimal import *
from array import array

try: xrange
except: xrange = range

exePath = os.path.dirname(os.path.abspath(inspect.getframeinfo(inspect.currentframe()).filename))

def wout(msg):
	sys.stdout.write(str(msg) + "\n")

# Efficient Spherical Harmonic Evaluation
# http://www.ppsloan.org/publications/SHJCGT.pdf
# http://jcgt.org/published/0002/02/06/

def shCalcKFast(l, m):
	am = abs(m);
	v = 1.0
	for k in xrange(l + am, l - am, -1): v *= float(k)
	#for k in xrange(l - am + 1, l + am + 1, 1): v *= float(k)
	return sqrt((2.0*l + 1.0) / (4.0*pi*v))

def shCalcKExact(l, m):
	am = abs(m);
	v = 1
	for k in xrange(l + am, l - am, -1): v *= k
	d = Decimal(2.0*l + 1.0) / (Decimal(v)*Decimal(4.0*pi))
	return d.sqrt()

def shCalcKPrecise(l, m):
	return float(shCalcKExact(l, m))

def shCalcK(l, m):
	if l < 87: return shCalcKFast(l, m)
	return shCalcKPrecise(l, m)

def shCalcPmm(m):
	v = 1
	for k in xrange(m+1): v *= 1 - 2*k
	return v

def shIdx(l, m): return l*(l+1) + m;

def shBandFromIdx(idx): return int(sqrt(idx))

def shNumCoefs(order):
	if order < 1: return 0
	return order*order

def shNumConsts(order):
	if order < 1: return 0
	return shNumCoefs(order) - (order-1)

def shCalcConstA(m):
	if m > 140:
		d = Decimal(shCalcPmm(m)) * shCalcKExact(m, m)
		return float(d) * sqrt(2.0)
	return shCalcPmm(m) * shCalcK(m, m) * sqrt(2.0)

def shCalcConstB(m):
	if m > 140:
		d = Decimal(shCalcPmm(m)) * shCalcKExact(m+1, m)
		return (2*m + 1) * float(d)
	return float(2*m + 1) * shCalcPmm(m) * shCalcK(m+1, m)

def shCalcConstC1(l, m):
	try:
		return shCalcK(l, m) / shCalcK(l-1, m) * float(2*l-1) / float(l-m)
	except:
		d = shCalcKExact(l, m) / shCalcKExact(l-1, m) * Decimal(2*l-1) / Decimal(l-m)
		return float(d)

def shCalcConstC2(l, m):
	try:
		return -shCalcK(l, m) / shCalcK(l-2, m) * float(l+m-1) / float(l-m)
	except:
		d = -shCalcKExact(l, m) / shCalcKExact(l-2, m) * Decimal(l+m-1) / Decimal(l-m)
		return float(d)

def shCalcConstD1(m):
	if m > 140:
		d = Decimal(shCalcPmm(m)) / Decimal(2.0) * shCalcKExact(m+2, m)
		return float((2*m+3)*(2*m+1)) * float(d)
	return float((2*m+3)*(2*m+1)) * shCalcPmm(m) / 2.0 * shCalcK(m+2, m)

def shCalcConstD2(m):
	if m > 140:
		d = Decimal(shCalcPmm(m)) / Decimal(2.0) * shCalcKExact(m+2, m)
		return float(-(2*m + 1)) * float(d)
	return float(-(2*m + 1)) * shCalcPmm(m) / 2.0 * shCalcK(m+2, m)

def shCalcConstE1(m):
	if m > 140:
		d = Decimal(shCalcPmm(m)) / Decimal(6.0) * shCalcKExact(m+3,m)
		return float((2*m+5)*(2*m+3)*(2*m+1)) * float(d)
	return float((2*m+5)*(2*m+3)*(2*m+1)) * shCalcPmm(m) / 6.0 * shCalcK(m+3,m)

def shCalcConstE2(m):
	if m > 140:
		pmm = Decimal(shCalcPmm(m))
		d = (Decimal((2*m+5)*(2*m+1))*pmm/Decimal(6.0) + Decimal((2*m+2)*(2*m+1))*pmm/Decimal(3.0)) * -shCalcKExact(m+3, m)
		return float(d)
	pmm = shCalcPmm(m)
	return (float((2*m+5)*(2*m+1))*pmm/6.0 + float((2*m+2)*(2*m+1))*pmm/3.0) * -shCalcK(m+3, m)

def shCalcConsts(order = 10):
	c = []
	if order < 1: return c
	# 0, 0
	c.append(shCalcK(0, 0))
	if order > 1:
		# 1, 0
		c.append(shCalcPmm(0)*shCalcK(1, 0))
	if order > 2:
		# 2, 0
		c.append(shCalcConstD1(0))
		c.append(shCalcConstD2(0))
	if order > 3:
		# 3, 0
		c.append(shCalcConstE1(0))
		c.append(shCalcConstE2(0))
	# 4.., 0
	for l in xrange(4, order):
		c.append(shCalcConstC1(l, 0))
		c.append(shCalcConstC2(l, 0))

	scl = sqrt(2.0)
	for m in xrange(1, order-1):
		l = m
		c.append(shCalcConstA(m))
		if m+1 < order:
			l += 1
			c.append(shCalcConstB(m) * scl)
		if m+2 < order:
			l += 1
			c.append(shCalcConstD1(m) * scl)
			c.append(shCalcConstD2(m) * scl)
		if m+3 < order:
			l += 1
			c.append(shCalcConstE1(m) * scl)
			c.append(shCalcConstE2(m) * scl)
		for l in xrange(m+4, order):
			c.append(shCalcConstC1(l, m))
			c.append(shCalcConstC2(l, m))
	if order > 1:
		c.append(shCalcConstA(order-1))
	return c

def saveConstsInc(path, consts):
	f = open(path, "w")
	if not f: return
	wout("Saving to " + path)
	for cval in consts: f.write(str(cval) + ",\12")
	f.close()

def coefPut(lang, idx, expr):
	lang.statement([Coef(idx), Op("=")] + expr)

def coefSum(lang, idx, expr):
	lang.statement([Coef(idx), Op("=")] + expr)
	for e in xrange(lang.applyNum):
		resIdx = ""
		if lang.applyNum > 1:
			resIdx = "[" + str(e) + "]"
		lang.statement([Var("res" + str(e)), Op("+="), Var("shc["+str(idx)+"]"), Op("*"), Var(lang.coefsName+"["+str(idx)+"]"+resIdx), Op("*"), Var("shw["+str(shBandFromIdx(idx))+"]"+resIdx)])

class Codegen:
	def __init__(self, order = 10):
		self.order = order
		self.consts = shCalcConsts(order)

	def genEval(self, lang):
		self.gen(lang, True)

	def genApply(self, lang):
		self.gen(lang, False)

	def gen(self, lang, isEval):
		lang.prologue(self.order, isEval)
		cidx = 0

		if isEval:
			cfn = coefPut
		else:
			cfn = coefSum

		cfn(lang, 0, [self.consts[cidx]])
		cidx += 1

		if self.order > 1:
			cfn(lang, shIdx(1, 0), [self.consts[cidx], Op("*"), Var("z")])
			cidx += 1

		if self.order > 2:
			cfn(lang, shIdx(2, 0), [self.consts[cidx], Op("*"), Var("zz"), Op("+"), self.consts[cidx + 1]])
			cidx += 2

		if self.order > 3:
			cfn(lang, shIdx(3, 0), [Op("("), self.consts[cidx], Op("*"), Var("zz"), Op("+"), self.consts[cidx + 1], Op(")"), Op("*"), Var("z")])
			cidx += 2

		for l in xrange(4, self.order):
			cfn(lang, shIdx(l, 0), [self.consts[cidx], Op("*"), Var("z"), Op("*"), Coef(shIdx(l-1, 0)), Op("+"), self.consts[cidx+1], Op("*"), Coef(shIdx(l-2, 0))])
			cidx += 2

		scIdx = 0
		for m in xrange(1, self.order-1):
			l = m
			s = "s" + str(scIdx)
			c = "c" + str(scIdx)

			lang.statement([Var("tmp"), Op("="), self.consts[cidx]])
			cidx += 1
			cfn(lang, l*l + l - m, [Var("tmp"), Op("*"), Var(s)])
			cfn(lang, l*l + l + m, [Var("tmp"), Op("*"), Var(c)])

			if m+1 < self.order:
				l = m+1
				lang.statement([Var("prev1"), Op("="), self.consts[cidx], Op("*"), Var("z")])
				cidx += 1
				cfn(lang, l*l + l - m, [Var("prev1"), Op("*"), Var(s)])
				cfn(lang, l*l + l + m, [Var("prev1"), Op("*"), Var(c)])

			if m+2 < self.order:
				l = m+2
				lang.statement([ Var("prev2"), Op("="), self.consts[cidx], Op("*"), Var("zz"), Op("+"), self.consts[cidx+1] ])
				cidx += 2
				cfn(lang, l*l + l - m, [Var("prev2"), Op("*"), Var(s)])
				cfn(lang, l*l + l + m, [Var("prev2"), Op("*"), Var(c)])

			if m+3 < self.order:
				l = m+3
				lang.statement([ Var("tmp"), Op("="), self.consts[cidx], Op("*"), Var("zz"), Op("+"), self.consts[cidx+1] ])
				cidx += 2
				lang.statement([ Var("prev0"), Op("="), Var("tmp"), Op("*"), Var("z") ])
				cfn(lang, l*l + l - m, [Var("prev0"), Op("*"), Var(s)])
				cfn(lang, l*l + l + m, [Var("prev0"), Op("*"), Var(c)])

			prevMask = 1 | (0 << 2) | (2 << 4) | (1 << 6) | (0 << 8) | (2 << 10) | (1 << 12)
			mask = prevMask
			maskCnt = 0
			for l in xrange(m+4, self.order):
				prevIdx = mask & 3
				lang.statement([Var("tmp"), Op("="), self.consts[cidx], Op("*"), Var("z")])
				cidx += 1
				lang.statement([ Var("prev"+str(prevIdx)), Op("="), Var("tmp"), Op("*"), Var("prev" + str((mask >> 2) & 3)) ])
				lang.statement([ Var("tmp"), Op("="), self.consts[cidx], Op("*"),  Var("prev" + str((mask >> 4) & 3)) ])
				cidx += 1
				lang.statement([ Var("prev"+str(prevIdx)), Op("+="), Var("tmp") ])
				cfn(lang, l*l + l - m, [Var("prev"+str(prevIdx)), Op("*"), Var(s)])
				cfn(lang, l*l + l + m, [Var("prev"+str(prevIdx)), Op("*"), Var(c)])

				maskCnt += 1
				if maskCnt < 3:
					mask >>= 4
				else:
					mask = prevMask
					maskCnt = 0
			lang.sincos(scIdx)
			scIdx ^= 1

		if self.order > 1:
			s = "s" + str(scIdx)
			c = "c" + str(scIdx)
			lang.statement([Var("tmp"), Op("="), self.consts[cidx]])
			cidx += 1
			cfn(lang, shIdx(self.order-1, -(self.order-1)), [Var("tmp"), Op("*"), Var(s)])
			cfn(lang, shIdx(self.order-1, (self.order-1)), [Var("tmp"), Op("*"), Var(c)])

		lang.epilogue(isEval)


EOL = "\n"

class Coef:
	def __init__(self, idx): self.idx = idx

class Var:
	def __init__(self, name): self.name = name

class Op:
	def __init__(self, op): self.op = op

class LangCBase:
	def __init__(self):
		self.code = ""
		self.coefsType = "float"
		self.coefsName = "pSH"
		self.floatPostfix = "f"
		self.applyType = "float"
		self.applyNum = 1

	def locals(self):
		self.code += "\t" + self.coefsType + " zz = z*z;" + EOL
		self.code += "\t" + self.coefsType + " tmp, prev0, prev1, prev2, s0 = y, s1, c0 = x, c1;" + EOL

	def expr(self, expr):
		for e in expr:
			tname = e.__class__.__name__
			if tname == "Coef":
				self.code += self.coefsName + "[" + str(e.idx) + "]"
			elif tname == "Var":
				self.code += e.name
			elif tname == "Op":
				self.code += " " + e.op + " "
			elif tname == "float":
				self.code += str(e) + self.floatPostfix

	def sincos(self, scIdx):
		self.code += "\t" + sinExpr(scIdx) + ";" + EOL
		self.code += "\t" + cosExpr(scIdx) + ";" + EOL

	def xyzParams(self):
		return self.coefsType + " x, " + self.coefsType + " y, " + self.coefsType + " z"

def sinExpr(scIdx):
	return "s" + str(scIdx^1) + " = x*s" + str(scIdx) + " + y*c" + str(scIdx)

def cosExpr(scIdx):
	return "c" + str(scIdx^1) + " = x*c" + str(scIdx) + " - y*s" + str(scIdx)

class LangHLSL(LangCBase):
	def __init__(self):
		LangCBase.__init__(self)
		self.coefsType = "float"
		self.coefsName = "sh"
		self.floatPostfix = ""
		self.applyType = "float3"

	def prologue(self, order, isEval):
		if isEval:
			self.code += "void shEval" + str(order) + "(out " + self.coefsType + " " + self.coefsName + "[" + str(shNumCoefs(order)) + "], " + self.xyzParams() +  ") {" + EOL
		else:
			self.code += "void shApply" + str(order) + "("
			for e in xrange(self.applyNum):
				self.code += "out " + self.applyType + " res" + str(e) + ", "
			self.code += self.applyType + " shc[" + str(shNumCoefs(order)) + "], "
			self.code += self.coefsType + " shw[" + str(order) + "], "
			self.code += self.xyzParams() +  ") {" + EOL
		LangCBase.locals(self)
		if not isEval:
			self.code += "\t" + self.coefsType + " " + self.coefsName + "[" + str(shNumCoefs(order)) + "];" + EOL
			for e in xrange(self.applyNum):
				self.code += "\tres" + str(e) + " = 0;" + EOL

	def statement(self, expr):
		self.code += "\t"
		LangCBase.expr(self, expr)
		self.code += ";" + EOL

	def epilogue(self, isEval):
		self.code += "}" + EOL

	def getFileExt(self): return ".hlsl"

class LangHLSL2(LangHLSL):
	def __init__(self):
		LangHLSL.__init__(self)
		self.coefsType = "float2"
		self.applyNum = 2

class LangCpp(LangCBase):
	def __init__(self): LangCBase.__init__(self)

	def prologue(self, order, isEval):
		self.code += "void shEval" + str(order) + "(" + self.coefsType + "* " + self.coefsName + ", " + self.xyzParams() + ") {" + EOL
		LangCBase.locals(self)

	def statement(self, expr):
		self.code += "\t"
		LangCBase.expr(self, expr)
		self.code += ";" + EOL

	def epilogue(self, isEval):
		self.code += "}" + EOL

	def getFileExt(self): return ".cpp"

class LangVEX(LangCBase):
	def __init__(self):
		LangCBase.__init__(self)
		self.coefsName = "sh"
		self.floatPostfix = ""

	def prologue(self, order, isEval):
		self.code += "function " + self.coefsType + "[] shEval" + str(order) + "(" + self.coefsType + " x, y, z) {" + EOL
		LangCBase.locals(self)
		self.code += "\t" + self.coefsType + " " + self.coefsName + "[];" + EOL
		self.code += "\tresize(" + self.coefsName + ", " + str(shNumCoefs(order)) + ");" + EOL

	def statement(self, expr):
		self.code += "\t"
		LangCBase.expr(self, expr)
		self.code += ";" + EOL

	def epilogue(self, isEval):
		self.code += "\treturn " + self.coefsName + ";" + EOL
		self.code += "}" + EOL

	def getFileExt(self): return ".vex"

class LangVEX3(LangVEX):
	def __init__(self):
		LangVEX.__init__(self)
		self.coefsType = "vector"

class LangVEX4(LangVEX):
	def __init__(self):
		LangVEX.__init__(self)
		self.coefsType = "vector4"


def genWgtSumHLSL(order, nelems = 2, elemType = "float3"):
	if order < 2: return
	ncoef = shNumCoefs(order)
	code = "void shWgtDot" + str(order) + "("
	for e in xrange(nelems):
		code += "out " + elemType + " res" + str(e) + ", "
	shtype = "float"
	if nelems > 1: shtype += str(nelems)
	code += elemType + " shc[" + str(ncoef) + "], "
	code += shtype + " sh[" + str(ncoef) + "], "
	code += shtype + " shw[" + str(order) + "]"
	code += ") {" + EOL
	for e in xrange(nelems):
		code += "\tres" + str(e) + " = 0;" + EOL
	for o in xrange(order):
		code += "\t// ~ band " + str(o) + " ~" + EOL
		i0 = shNumCoefs(o)
		i1 = shNumCoefs(o+1)
		m = -o
		for i in xrange(i0, i1):
			code += "\t// (" + str(o) + ", " + str(m) + ")" + EOL
			for e in xrange(nelems):
				eIdx = ""
				if nelems > 1:
					eIdx = "[" + str(e) + "]"
				code += "\tres" + str(e) + " += shc[" + str(i) + "] * sh[" + str(i) + "]" + eIdx + " * shw[" + str(o) + "]" + eIdx + ";" + EOL
			m += 1
	code += "}" + EOL
	return code

def genGetHLSL(order, nwgt = 2, gpname = "g_sh"):
	if order < 2: return
	unrollFlg = order < 20
	ncoef = shNumCoefs(order)
	code = "void shGet" + str(order) + "("
	wtype = "float"
	if nwgt > 1: wtype += str(nwgt)
	code += "out float3 shc[" + str(ncoef) + "], out " + wtype + " shw[" + str(order) + "]) {" + EOL
	code += "\tint i;" + EOL
	if unrollFlg: code += "\t[unroll(" + str(ncoef) + ")]" + EOL
	code += "\tfor (i = 0; i < " + str(ncoef) + "; ++i) {" + EOL
	code += "\t\tshc[i] = " + gpname + "[i].rgb;" + EOL;
	code += "\t}" + EOL
	for widx in xrange(nwgt):
		widxStr = ""
		if nwgt > 1: widxStr = "[" + str(widx) + "]"
		if unrollFlg: code += "\t[unroll(" + str(order) + ")]" + EOL
		code += "\tfor (i = 0; i < " + str(order) + "; ++i) {" + EOL
		code += "\t\tshw[i]" + widxStr + " = " + gpname + "[i+" + str(widx*order) + "].a;" + EOL
		code += "\t}" + EOL
	code += "}" + EOL
	return code

def saveStr(path, s):
	f = open(path, "w")
	if not f: return
	wout("Saving to " + path)
	f.write(s)
	f.close()

def genHLSL():
	outPath = exePath + "/sheval.h"
	f = open(outPath, "w")
	if not f: return
	maxOrder = 19
	f.write("// float" + EOL)
	for order in xrange(2, maxOrder+1):
		cg = Codegen(order)
		lang = LangHLSL()
		cg.genEval(lang)
		f.write(lang.code)
		f.write(EOL)
	f.write(EOL)
	f.write(EOL)
	f.write("// float2" + EOL)
	for order in xrange(2, maxOrder+1):
		cg = Codegen(order)
		lang = LangHLSL2()
		cg.genEval(lang)
		f.write(lang.code)
		f.write(EOL)
	f.write(EOL)
	f.write(EOL)

	f.write("// wdot" + EOL)
	for order in xrange(2, maxOrder+1):
		code = genWgtSumHLSL(order, 1)
		f.write(code)
		f.write(EOL)
	f.write(EOL)

	f.write("// wdot2" + EOL)
	for order in xrange(2, maxOrder+1):
		code = genWgtSumHLSL(order, 2)
		f.write(code)
		f.write(EOL)
	f.write(EOL)

	f.write("// apply" + EOL)
	for order in xrange(2, maxOrder+1):
		cg = Codegen(order)
		lang = LangHLSL()
		cg.genApply(lang)
		f.write(lang.code)
		f.write(EOL)
	f.write(EOL)

	f.write("// apply2" + EOL)
	for order in xrange(2, maxOrder+1):
		cg = Codegen(order)
		lang = LangHLSL2()
		cg.genApply(lang)
		f.write(lang.code)
		f.write(EOL)
	f.write(EOL)

	f.write("// get" + EOL)
	for order in xrange(2, maxOrder+1):
		code = genGetHLSL(order, 1)
		f.write(code)
		f.write(EOL)
	f.write(EOL)

	f.write("// get2" + EOL)
	for order in xrange(2, maxOrder+1):
		code = genGetHLSL(order, 2)
		f.write(code)
		f.write(EOL)
	f.write(EOL)

	f.close()

if __name__=="__main__":
	genHLSL()

