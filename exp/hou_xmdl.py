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

def texPath(path):
	if path.startswith("op:"): return path[3:]
	return path

def packXMtx(mtx):
	return xcore.packTupF32(mtx.transposed().asTuple()[:3*4])

class MdlMaterial:
	def __init__(self, mdl, path):
		self.mdl = mdl
		self.path = path
		self.pathId = -1
		self.nameId = -1
		self.setDefaultParams()
		self.node = None
		self.name = self.path
		if path:
			sep = self.path.rfind("/")
			if sep >= 0:
				self.pathId = self.mdl.strLst.add(self.path[:sep])
				self.name = self.name[sep+1:]
			self.node = hou.node(path)
			if self.node:
				mtlType = self.node.type().name()
				if mtlType == "classicshader":
					if self.node.parm("diff_colorUseTexture").evalAsInt():
						self.baseMapPath = texPath(self.node.parm("diff_colorTexture").evalAsString())
					self.baseColor = self.node.parmTuple("diff_color").eval()
					if self.node.parm("baseBumpAndNormal_enable").evalAsInt():
						self.bumpMapPath = texPath(self.node.parm("baseNormal_texture").evalAsString())
					if self.node.parm("refl_colorUseTexture").evalAsInt():
						self.specMapPath = texPath(self.node.parm("refl_colorTexture").evalAsString())
					if self.node.parm("refl_roughUseTexture").evalAsInt():
						tpath = self.node.parm("refl_roughTexture").evalAsString()
						if tpath and tpath.endswith("_SURF"):
							self.surfMapPath = texPath(tpath)
					elif self.node.parm("refl_metallicUseTexture").evalAsInt():
						tpath = self.node.parm("refl_metallicTexture").evalAsString()
						if tpath and tpath.endswith("_SURF"):
							self.surfMapPath = texPath(tpath)
					self.specColor = self.node.parmTuple("spec_color").eval()
					self.roughness = self.node.parm("spec_rough").evalAsFloat()
					self.metallic = self.node.parm("spec_metallic").evalAsFloat()
					self.fresnel = xcore.fresnelFromIOR(self.node.parm("ior_in").evalAsFloat())
					self.bumpScale = self.node.parm("baseNormal_scale").evalAsFloat()
					self.setFlgUseAlpha(self.node.parm("diff_colorTextureUseAlpha").evalAsInt())
					self.setFlgFlipTangent(self.node.parm("baseNormal_flipX").evalAsInt())
					self.setFlgFlipBitangent(self.node.parm("baseNormal_flipY").evalAsInt())
				elif mtlType == "principledshader":
					if self.node.parm("basecolor_useTexture").evalAsInt():
						self.baseMapPath = texPath(self.node.parm("basecolor_texture").evalAsString())
					self.baseColor = self.node.parmTuple("basecolor").eval()

				for prmName in ["gex_dblSided", "dmd_dblsided"]:
					prm = self.node.parm(prmName)
					if prm:
						self.setFlgDblSided(prm.evalAsInt())
						break
				for prmName in ["dmd_forceblend"]:
					prm = self.node.parm(prmName)
					if prm:
						self.setFlgForceBlend(prm.evalAsInt())
						break
				for prmName in ["dmd_alphalim"]:
					prm = self.node.parm(prmName)
					if prm:
						self.alphaLim = prm.evalAsFloat()
						break
				for prmName in ["dmd_sdwalphalim"]:
					prm = self.node.parm(prmName)
					if prm:
						self.shadowAlphaLim = prm.evalAsFloat()
						break
				for prmName in ["dmd_bmsalpha"]:
					prm = self.node.parm(prmName)
					if prm:
						self.setFlgBaseMapSpecAlpha(prm.evalAsInt())
						break
				for prmName in ["dmd_scast"]:
					prm = self.node.parm(prmName)
					if prm:
						self.setFlgShadowCast(prm.evalAsInt())
						break
				for prmName in ["dmd_srecv"]:
					prm = self.node.parm(prmName)
					if prm:
						self.setFlgShadowRecv(prm.evalAsInt())
						break
				for prmName in ["dmd_soffs"]:
					prm = self.node.parm(prmName)
					if prm:
						self.shadowOffs = prm.evalAsFloat()
						break
				for prmName in ["dmd_swght"]:
					prm = self.node.parm(prmName)
					if prm:
						self.shadowWght = prm.evalAsFloat()
						break
				for prmName in ["dmd_sdens"]:
					prm = self.node.parm(prmName)
					if prm:
						self.shadowDensity = prm.evalAsFloat()
						break
				for prmName in ["dmd_diffrough"]:
					prm = self.node.parm(prmName)
					if prm:
						self.setFlgDiffRoughness(prm.evalAsInt())
						break

				if self.baseMapPath:
					tpath = None
					sep = self.baseMapPath.rfind("/")
					if sep >= 0:
						tpath = self.baseMapPath[:sep]
						tname = self.baseMapPath[sep+1:]
					else:
						tname = self.baseMapPath
					self.baseTexId = self.mdl.addTex(tname, tpath)
				if self.specMapPath:
					tpath = None
					sep = self.specMapPath.rfind("/")
					if sep >= 0:
						tpath = self.specMapPath[:sep]
						tname = self.specMapPath[sep+1:]
					else:
						tname = self.specMapPath
					self.specTexId = self.mdl.addTex(tname, tpath)
				if self.bumpMapPath:
					tpath = None
					sep = self.bumpMapPath.rfind("/")
					if sep >= 0:
						tpath = self.bumpMapPath[:sep]
						tname = self.bumpMapPath[sep+1:]
					else:
						tname = self.bumpMapPath
					self.bumpTexId = self.mdl.addTex(tname, tpath)
				if self.surfMapPath:
					tpath = None
					sep = self.surfMapPath.rfind("/")
					if sep >= 0:
						tpath = self.surfMapPath[:sep]
						tname = self.surfMapPath[sep+1:]
					else:
						tname = self.surfMapPath
					self.surfTexId = self.mdl.addTex(tname, tpath)
		self.nameId = self.mdl.strLst.add(self.getName())
		self.swapPaths = []
		self.swapIds = None
		if self.node:
			swpName = "dmd_mtlswap"
			for prm in self.node.allParms():
				if prm.name().startswith(swpName):
					self.swapPaths.append(prm.evalAsString())

	def setDefaultParams(self):
		self.baseColor = [1.0, 1.0, 1.0]
		self.baseTexId = -1
		self.baseMapPath = None
		self.specColor = [1.0, 1.0, 1.0]
		self.specTexId = -1
		self.specMapPath = None
		self.roughness = 0.75
		self.metallic = 0.0
		self.surfTexId = -1
		self.surfMapPath = None
		self.fresnel = xcore.fresnelFromIOR(1.33)
		self.bumpTexId = -1
		self.bumpMapPath = None
		self.bumpScale = 0.0
		self.alphaLim = 0.0
		self.flags = 0
		self.setFlgShadowCast(True)
		self.setFlgShadowRecv(True)
		self.shadowOffs = 0.0
		self.shadowWght = 0.0
		self.shadowDensity = 0.75
		self.shadowAlphaLim = 0.5
		self.baseMapSpecGain = 0.0
		self.baseMapSpecBias = 0.0
		self.baseMapSpecPwr = 1.0
		self.baseMapSpecAlpha = 0.0
		self.mask = 1.0
		self.sortBias = 0.0
		self.sortLayer = 0
		self.sortMode = 0 # near

	def setFlg(self, idx, val):
		mask = 1 << idx
		if val:
			self.flags |= mask
		else:
			self.flags &= ~mask

	def setFlgUseAlpha(self, flg): self.setFlg(0, flg)
	def setFlgDither(self, flg): self.setFlg(1, flg)
	def setFlgForceBlend(self, flg): self.setFlg(2, flg)
	def setFlgDblSided(self, flg): self.setFlg(3, flg)
	def setFlgFlipTangent(self, flg): self.setFlg(4, flg)
	def setFlgFlipBitangent(self, flg): self.setFlg(5, flg)
	def setFlgShadowCast(self, flg): self.setFlg(6, flg)
	def setFlgShadowRecv(self, flg): self.setFlg(7, flg)
	def setFlgBaseMapSpecAlpha(self, flg): self.setFlg(8, flg)
	def setFlgSortTris(self, flg): self.setFlg(9, flg)
	def setFlgDiffRoughness(self, flg): self.setFlg(10, flg)

	def getName(self):
		if self.name: return self.name
		return "$default"

	def hasSwaps(self):
		return self.swapPaths and len(self.swapPaths) > 0

	def write(self, bw):
		self.mdl.writeStrId32(bw, self.nameId) # +00
		self.mdl.writeStrId32(bw, self.pathId) # +04
		bw.writeI32(self.baseTexId) # +08
		bw.writeI32(self.specTexId) # +0C
		bw.writeI32(self.bumpTexId) # +10
		bw.writeI32(self.surfTexId) # +14
		bw.writeI32(-1) # +18 extTexId
		bw.writeU32(self.flags) # +1C
		bw.writeFV(self.baseColor) # +20
		bw.writeFV(self.specColor) # +2C
		bw.writeF32(self.roughness) # +38
		bw.writeF32(self.fresnel) # +3C
		bw.writeF32(self.metallic) # +40
		bw.writeF32(self.bumpScale) # +44
		bw.writeF32(self.alphaLim) # +48
		bw.writeF32(self.shadowOffs) # +4C
		bw.writeF32(self.shadowWght) # +50
		bw.writeF32(self.shadowDensity) # +54
		bw.writeF32(self.shadowAlphaLim) # +58
		bw.writeF32(self.mask) # +5C
		bw.writeU32(0) # +60 -> ext
		bw.writeU32(0) # +64 -> swaps
		bw.writeF16(self.sortBias) # +68
		bw.writeU8(self.sortMode) # +6A
		bw.writeI8(self.sortLayer) # +6B
		bw.writeU32(0) # +6C reserved
		bw.writeU32(0) # +70 wk
		bw.writeU32(0) # ...
		bw.writeU32(0) # ...
		bw.writeU32(0) # ...

MdlMaterial.SIZE = 0x80
MdlMaterial.SWAPS_OFFS = 0x64

class MdlSkelNode:
	def __init__(self, mdl, hnode):
		self.mdl = mdl
		self.hnode = hnode
		self.nameId = mdl.strLst.add(self.hnode.name())
		self.wmtx = self.hnode.worldTransform()
		self.lmtx = self.hnode.localTransform()
		self.imtx = self.wmtx.inverted()
		inp = self.hnode.inputConnectors()[0]
		if len(inp):
			self.parent = inp[0].inputNode()
		else:
			self.parent = None

class MdlPolGrp:
	def __init__(self, mdl):
		self.mdl = mdl
		self.imtl = 0
		self.ipols = []
		self.ijnts = None
		self.name = None

class MdlBatch:
	def __init__(self, mdl):
		self.mdl = mdl
		self.igrp = -1
		self.idiv = 0
		self.name = None
		self.ijnts = None
		self.org = 0
		self.npol = 0
		self.ntri = 0
		self.jntInfoOffs = -1
		self.nameId = -1
		self.minIdx = -1
		self.maxIdx = -1
		self.idxOrg = 0

	def numJnts(self):
		if self.ijnts: return len(self.ijnts)
		return 0

	def jntLstFromMask(self, jmask):
		nskin = self.mdl.numSkin()
		self.ijnts = []
		for ijnt in xrange(nskin):
			if jmask & (1 << ijnt):
				self.ijnts.append(ijnt)

	def getPntLst(self):
		if self.igrp < 0: return None
		lst = []
		grp = self.mdl.grps[self.igrp]
		for i in xrange(self.npol):
			ipol = grp.ipols[self.org + i]
			pol = self.mdl.pols[ipol]
			lst += pol.pids
		return list(set(lst))

	def calcNumTris(self):
		self.ntri = 0
		grp = self.mdl.grps[self.igrp]
		for i in xrange(self.npol):
			ipol = grp.ipols[self.org + i]
			pol = self.mdl.pols[ipol]
			self.ntri += pol.numTris()

	def calcIdxRange(self):
		grp = self.mdl.grps[self.igrp]
		ipol = grp.ipols[self.org]
		pol = self.mdl.pols[ipol]
		self.minIdx = pol.minIdx()
		self.maxIdx = pol.maxIdx()
		for i in xrange(1, self.npol):
			ipol = grp.ipols[self.org + i]
			pol = self.mdl.pols[ipol]
			self.minIdx = min(self.minIdx, pol.minIdx())
			self.maxIdx = max(self.maxIdx, pol.maxIdx())

	def isIdx16(self):
		return (self.maxIdx - self.minIdx) < (1 << 16)

	def getJntPids(self, ijnt):
		jpids = []
		if self.mdl.skinData:
			pids = self.getPntLst()
			for ipnt in pids:
				skn = self.mdl.skinData[ipnt]
				for iw in skn:
					if iw[0] == ijnt:
						jpids.append(ipnt)
						break
		return jpids

	def write(self, bw):
		grp = self.mdl.grps[self.igrp]
		self.mdl.writeStrId32(bw, self.nameId) # +00
		bw.writeI32(grp.imtl) # +04
		bw.writeI32(self.minIdx) # +08
		bw.writeI32(self.maxIdx) # +0C
		bw.writeI32(self.idxOrg) # +10
		bw.writeI32(self.ntri) # +14
		bw.writeI32(self.numJnts()) # +18
		bw.writeI32(self.jntInfoOffs) # +1C

class MdlJntSphere:
	def __init__(self, mdl):
		self.mdl = mdl

	def start(self, ijnt):
		self.ijnt = ijnt
		self.radius = 0.0
		self.center = [0.0 for i in xrange(3)]
		self.count = 0
		self.pids = []
		self.imin = [0 for i in xrange(3)]
		self.imax = [0 for i in xrange(3)]

	def addPnt(self, ipnt):
		if not self.mdl.skinData: return
		skin = self.mdl.skinData[ipnt]
		for iw in skin:
			if iw[0] == self.ijnt:
				if self.count > 0:
					pos = self.mdl.ppos[ipnt]
					for i in xrange(3):
						if pos[i] < self.mdl.ppos[self.imin[i]][i]: self.imin[i] = ipnt
						if pos[i] > self.mdl.ppos[self.imax[i]][i]: self.imax[i] = ipnt
				else:
					self.imin = [ipnt for i in xrange(3)]
					self.imax = [ipnt for i in xrange(3)]
				self.pids.append(ipnt)
				self.count += 1
				break

	def calc(self, adjust = True):
		v = [None, None, None]
		for i in xrange(3):
			v[i] = [self.mdl.ppos[self.imax[i]][j] - self.mdl.ppos[self.imin[i]][j] for j in xrange(3)]
		d = [0, 0, 0]
		for i in xrange(3):
			d[i] = sum([v[i][j]*v[i][j] for j in xrange(3)])
		pidMin = self.imin[0]
		pidMax = self.imax[0]
		if d[1] > d[0] and d[1] > d[2]:
			pidMin = self.imin[1]
			pidMax = self.imax[1]
		if d[2] > d[0] and d[2] > d[1]:
			pidMin = self.imin[2]
			pidMax = self.imax[2]
		pmin = self.mdl.ppos[pidMin]
		pmax = self.mdl.ppos[pidMax] 
		c = [(pmin[i] + pmax[i]) * 0.5 for i in xrange(3)]
		e = [pmax[i] - c[i] for i in xrange(3)]
		r = sqrt(sum([e[i]*e[i] for i in xrange(3)]))
		for pid in self.pids:
			p = self.mdl.ppos[pid]
			v = [p[i] - c[i] for i in xrange(3)]
			dd = sum([v[i]*v[i] for i in xrange(3)])
			if dd > r*r:
				d = sqrt(dd)
				nr = (r + d) * 0.5
				s = (nr - r) / d
				r = nr
				c = [c[i] + v[i]*s for i in xrange(3)]
		if adjust:
			for pid in self.pids:
				p = self.mdl.ppos[pid]
				v = [p[i] - c[i] for i in xrange(3)]
				dd = sum([v[i]*v[i] for i in xrange(3)])
				if dd > r*r:
					r = sqrt(dd) + r*1.0e-5
		self.center = c
		self.radius = r

class MdlPntAttrs:
	def __init__(self, mdl, alfAttrName = "Alpha"):
		self.mdl = mdl
		self.nrm = self.mdl.geo.findPointAttrib("N")
		self.clr = self.mdl.geo.findPointAttrib("Cd")
		self.tex = self.mdl.geo.findPointAttrib("uv")
		self.alf = self.mdl.geo.findPointAttrib(alfAttrName)

	def getNrm(self, pnt):
		if self.nrm: return pnt.attribValue(self.nrm)
		return [0.0, 0.0, 1.0]

	def getClr(self, pnt):
		if self.clr: return pnt.attribValue(self.clr)
		return [1.0, 1.0, 1.0]

	def getTex(self, pnt):
		if self.tex: return pnt.attribValue(self.tex)
		return [0.0, 0.0, 1.0]

	def getAlf(self, pnt):
		if self.alf: return pnt.attribValue(self.alf)
		return 1.0

class MdlExporter(xcore.BaseExporter, xhou.PolModel):
	def __init__(self):
		xcore.BaseExporter.__init__(self)
		xhou.PolModel.__init__(self)
		self.sig = "XMDL"
		self.isStatic = False
		self.useHalf = False
		self.exportName = None

	def build(self, sop, usePrimGroups = False, batPrefix = None, batJntsLimit = 16, skelRootPath = None, alfAttrName = "SpecMask"):
		self.bboxMin = [0.0 for i in xrange(3)]
		self.bboxMax = [0.0 for i in xrange(3)]
		xhou.PolModel.build(self, sop)
		if self.exportName:
			#xcore.dbgmsg("Exporting as '" + self.exportName + "'")
			self.setNameFromPath(self.exportName)
		else:
			self.setNameFromPath(self.sop.parent().path())
		self.skelRootPath = skelRootPath
		self.usePrimGroups = usePrimGroups
		self.batPrefix = batPrefix
		self.batJntsLimit = batJntsLimit
		self.alfAttrName = alfAttrName
		self.tlst = []
		self.tdict = {}
		self.initMtls()
		self.initSkin()
		self.initSkel()
		self.initGrps()
		self.initBats()
		self.initCull()

	def addTex(self, name, path):
		if not name: return -1
		pathId = -1
		nameId = self.strLst.add(name)
		if path:
			pathId = self.strLst.add(path)
			key = path + "/" + name
		else:
			key = name
		if not key in self.tdict:
			tid = len(self.tlst)
			self.tlst.append([nameId, pathId])
			self.tdict[key] = tid
		return self.tdict[key]


	def initMtls(self):
		xhou.PolModel.initMtls(self)
		self.mtls = []
		for mtlPath in self.mtlPaths:
			self.mtls.append(MdlMaterial(self, mtlPath))
		swpLst = []
		for imtl, mtl in enumerate(self.mtls):
			if mtl.hasSwaps():
				for iswp, swpPath in enumerate(mtl.swapPaths):
					swpLst.append([imtl, iswp, swpPath])
		self.swpMtls = []
		if len(swpLst) > 0:
			swpMap = {}
			for swp in swpLst:
				swpPath = swp[2]
				if not swpPath in swpMap:
					swpMtlId = len(self.swpMtls)
					swpMap[swpPath] = swpMtlId
					self.swpMtls.append(MdlMaterial(self, swpPath))
			for imtl, mtl in enumerate(self.mtls):
				if mtl.hasSwaps():
					mtl.swapIds = []
					for iswp, swpPath in enumerate(mtl.swapPaths):
						mtl.swapIds.append(swpMap[swpPath])

	def initSkin(self):
		self.skinNames = None
		self.skinData = None
		self.maxWgtPerPnt = 0
		skinAttr = self.geo.findPointAttrib("boneCapture")
		if skinAttr:
			self.skinNames = []
			self.skinNameToStrId = {}
			tbl = skinAttr.indexPairPropertyTables()[0]
			n = tbl.numIndices()
			for i in xrange(n):
				name = tbl.stringPropertyValueAtIndex("pCaptPath", i)
				name = name.split("/cregion")[0]
				nameId = self.strLst.add(name)
				self.skinNameToStrId[name] = nameId
				self.skinNames.append(name)
			self.skinData = []
			for ipnt, pnt in enumerate(self.pnts):
				skin = pnt.floatListAttribValue(skinAttr)
				nwgt = len(skin) // 2
				iw = []
				for i in xrange(nwgt):
					idx = int(skin[i*2])
					wgt = skin[i*2 + 1]
					if idx >= 0.0:
						if wgt > 0.0 and wgt < 1.0e-3: wgt = 0.0
						if wgt > 0.0:
							iw.append([idx, wgt])
						elif wgt < 0.0:
							xcore.dbgmsg("Warning: negative weight at point {}".format(ipnt))
				if len(iw) < 1:
					xcore.dbgmsg("Warning: unweighted point {}".format(ipnt))
					iw.append([0, 0.0])
				iw.sort(key = lambda iw: -iw[1])
				if len(iw) > 4:
					xcore.dbgmsg("Warning: too many weights at point {}".format(ipnt))
					iw = iw[:4]
				self.skinData.append(iw)
			for iskn, skn in enumerate(self.skinData):
				self.maxWgtPerPnt = max(self.maxWgtPerPnt, len(skn))
				s = 0.0
				for iw in skn:
					s += iw[1]
				if s and s != 1.0:
					s = 1.0 / s
					for iw in skn: iw[1] *= s

	def initSkel(self):
		self.skelNodes = None
		self.skelCHOPs = None
		if self.skinData and self.skelRootPath:
			skelRoot = hou.node(self.skelRootPath)
			if skelRoot:
				self.skelCHOPs = []
				self.skelTreeCHOPs(skelRoot)
				for chop in self.skelCHOPs: chop.setExportFlag(False)
				self.skelNodes = []
				self.skelNodeMap = {}
				self.skelTree(skelRoot)
				for skelNode in self.skelNodes:
					if skelNode.parent:
						skelNode.parentId = self.skelNodeMap[skelNode.parent.name()]
					else:
						skelNode.parentId = -1
				for chop in self.skelCHOPs: chop.setExportFlag(True)

	def skelTreeCHOPs(self, node):
		path = node.path()
		for ch in ["t", "r", "s"]:
			ct = hou.parmTuple(path + "/" + ch)
			if ct:
				for c in ct:
					trk = c.overrideTrack()
					if trk:
						chop = trk.chopNode()
						if not chop in self.skelCHOPs:
							self.skelCHOPs.append(chop)
		for link in node.outputConnectors()[0]:
			self.skelTreeCHOPs(link.outputNode())

	def skelTree(self, node):
		self.skelNodeMap[node.name()] = len(self.skelNodes)
		self.skelNodes.append(MdlSkelNode(self, node))
		for link in node.outputConnectors()[0]:
			self.skelTree(link.outputNode())

	def initGrps(self):
		if (len(self.pols)) < 1: return
		geoGrps = []
		if self.usePrimGroups:
			for grp in self.geo.primGroups():
				if not self.batPrefix or grp.name().startswith(self.batPrefix):
					geoGrps.append(grp)
		grps = []
		if len(geoGrps) < 1:
			xcore.dbgmsg("using mtl bats")
			nmtl = len(self.mtls)
			for i in xrange(nmtl):
				grp = MdlPolGrp(self)
				grp.imtl = i
				grp.name = self.mtls[i].getName()
				grps.append(grp)
			for ipol, pol in enumerate(self.pols):
				grps[pol.imtl].ipols.append(ipol)
		else:
			xcore.dbgmsg("using grp bats")
			primToPolIdx = {}
			for ipol, pol in enumerate(self.pols):
				primToPolIdx[pol.prim] = ipol
			for gg in geoGrps:
				grp = MdlPolGrp(self)
				grp.name = gg.name()
				for prim in gg.prims():
					#if prim in primToPolIdx:
					grp.ipols.append(primToPolIdx[prim])
				grp.imtl = self.pols[grp.ipols[0]].imtl
				grps.append(grp)
		ngrps = len(grps)
		xcore.dbgmsg(str(ngrps) + " polygon groups")
		nskin = self.numSkin()
		if nskin > 0:
			for grp in grps:
				grp.ijnts = []
				jmask = 0
				for ipol in grp.ipols:
					pol = self.pols[ipol]
					for ipnt in pol.pids:
						for iw in self.skinData[ipnt]:
							ijnt = iw[0]
							jmask |= 1 << ijnt
				for ijnt in xrange(nskin):
					if jmask & (1 << ijnt):
						grp.ijnts.append(ijnt)
		self.grps = grps

	def initBats(self):
		self.bats = []
		nskin = self.numSkin()
		if nskin > 0:
			if self.batJntsLimit > 0:
				for igrp, grp in enumerate(self.grps):
					njnts = len(grp.ijnts)
					if njnts > self.batJntsLimit:
						xcore.dbgmsg("splitting grp: " + grp.name + " (" + str(njnts) + " joints)")
						npol = len(grp.ipols)
						idx = 0
						org = 0
						idiv = 0
						jmask = 0
						while idx < npol:
							ipol = grp.ipols[idx]
							pol = self.pols[ipol]
							jmaskPrev = jmask
							for ipnt in pol.pids:
								for iw in self.skinData[ipnt]:
									ijnt = iw[0]
									jmask |= 1 << ijnt
							njnt = 0
							for ijnt in xrange(nskin):
								if jmask & (1 << ijnt): njnt += 1
							if njnt > self.batJntsLimit:
								bat = MdlBatch(self)
								bat.igrp = igrp
								bat.idiv = idiv
								bat.org = org
								bat.npol = idx - org
								bat.jntLstFromMask(jmaskPrev)
								bat.name = grp.name + "_" + str(bat.idiv)
								self.bats.append(bat)
								jmask = 0
								org = idx
								idiv += 1
							else:
								if idx == npol - 1:
									bat = MdlBatch(self)
									bat.igrp = igrp
									bat.idiv = idiv
									bat.org = org
									bat.npol = idx - org + 1
									bat.jntLstFromMask(jmask)
									bat.name = grp.name + "_" + str(bat.idiv)
									self.bats.append(bat)
								idx += 1
					else:
						bat = MdlBatch(self)
						bat.igrp = igrp
						bat.name = grp.name
						bat.org = 0
						bat.npol = len(grp.ipols)
						bat.ijnts = grp.ijnts
						self.bats.append(bat)
			else:
				for igrp, grp in enumerate(self.grps):
						bat = MdlBatch(self)
						bat.igrp = igrp
						bat.name = grp.name
						bat.org = 0
						bat.npol = len(grp.ipols)
						bat.ijnts = grp.ijnts
						self.bats.append(bat)
		else: # !skin
			for igrp, grp in enumerate(self.grps):
				bat = MdlBatch(self)
				bat.igrp = igrp
				bat.name = grp.name
				bat.org = 0
				bat.npol = len(grp.ipols)
				bat.ijnts = grp.ijnts
				self.bats.append(bat)

		for bat in self.bats:
			bat.nameId = self.strLst.add(bat.name)
			bat.calcNumTris()
			bat.calcIdxRange()

		self.nidx16 = 0
		self.nidx32 = 0
		for bat in self.bats:
			if bat.isIdx16():
				bat.idxOrg = self.nidx16
				self.nidx16 += bat.ntri * 3
			else:
				bat.idxOrg = self.nidx32
				self.nidx32 += bat.ntri * 3
		xcore.dbgmsg("#bats: " + str(self.numBat()))

	def initCull(self):
		self.jsphMdl = None
		self.jsphBats = None
		if len(self.bats) < 1: return

		for bat in self.bats:
			if bat.npol > 0:
				grp = self.grps[bat.igrp]
				ipol = grp.ipols[bat.org]
				pol = self.pols[ipol]
				bat.bboxMin = [pol.bboxMin[i] for i in xrange(3)]
				bat.bboxMax = [pol.bboxMax[i] for i in xrange(3)]
				for i in xrange(1, bat.npol):
					ipol = grp.ipols[bat.org + i]
					pol = self.pols[ipol]
					for j in xrange(3):
						bat.bboxMin[j] = min(bat.bboxMin[j], pol.bboxMin[j])
						bat.bboxMax[j] = max(bat.bboxMax[j], pol.bboxMax[j])
			else:
				bat.bboxMin = [0.0 for i in xrange(3)]
				bat.bboxMax = [0.0 for i in xrange(3)]

		nskin = self.numSkin()
		if nskin <= 0: return
		jsph = MdlJntSphere(self)
		npnt = len(self.pnts)
		self.jsphMdl = []
		for ijnt in xrange(nskin):
			jsph.start(ijnt)
			for ipnt in xrange(npnt): jsph.addPnt(ipnt)
			jsph.calc()
			self.jsphMdl += jsph.center
			self.jsphMdl.append(jsph.radius)
			#xcore.dbgmsg(self.skinNames[ijnt] + ": " + str(jsph.center) + " " + str(jsph.radius))
		self.jsphBats = []
		top = 0
		for bat in self.bats:
			njnt = bat.numJnts()
			if bat.npol > 0 and njnt > 0:
				pids = bat.getPntLst()
				bat.jntInfoOffs = top
				for ijnt in bat.ijnts:
					jsph.start(ijnt)
					for ipnt in pids: jsph.addPnt(ipnt)
					jsph.calc()
					self.jsphBats += jsph.center
					self.jsphBats.append(jsph.radius)
				top += njnt

	def numSkin(self):
		if self.skinNames: return len(self.skinNames)
		return 0

	def numSkel(self):
		if self.skelNodes: return len(self.skelNodes)
		return 0

	def numPnt(self):
		if self.pnts: return len(self.pnts)
		return 0

	def numBat(self):
		if self.bats: return len(self.bats)
		return 0

	def numMtl(self):
		if self.mtls: return len(self.mtls)
		return 0

	def writeHead(self, bw, top):
		npnt = self.numPnt()
		ntri = self.ntris
		nmtl = self.numMtl()
		nbat = self.numBat()
		nskn = self.numSkin()
		nskl = self.numSkel()
		ntex = len(self.tlst)

		maxJntsPerBat = 0
		if nskn > 0:
			for bat in self.bats:
				maxJntsPerBat = max(maxJntsPerBat, bat.numJnts())

		bw.writeFV(self.bboxMin) # +20
		bw.writeFV(self.bboxMax) # +2C
		bw.writeU32(npnt) # +38
		bw.writeU32(ntri) # +3C
		bw.writeU32(nmtl) # +40
		bw.writeU32(ntex) # +44 
		bw.writeU32(nbat) # +48
		bw.writeU32(nskn) # +4C
		bw.writeU32(nskl) # +50
		bw.writeU32(self.nidx16) # +54
		bw.writeU32(self.nidx32) # +58
		bw.writeU32(maxJntsPerBat) # +5C
		self.patchPos = bw.getPos()
		bw.writeU32(0) # +60 -> pnts
		bw.writeU32(0) # +64 -> mtls
		bw.writeU32(0) # +68 -> texs
		bw.writeU32(0) # +6C -> bats
		bw.writeU32(0) # +70 -> skin
		bw.writeU32(0) # +74 -> skel
		bw.writeU32(0) # +78 -> idx16
		bw.writeU32(0) # +7C -> idx32
		for i in xrange(8): bw.writeU32(0) # +80 GPU wk [32 bytes]


	def writeHalfVB(self, f):
		attrs = MdlPntAttrs(self, self.alfAttrName)
		npnt = self.numPnt()
		for ipnt in xrange(npnt):
			pnt = self.pnts[ipnt]
			pos = self.ppos[ipnt]
			nrm = attrs.getNrm(pnt)
			onrm = xcore.encodeOcta(nrm)
			tex = attrs.getTex(pnt)
			clr = attrs.getClr(pnt)
			alf = attrs.getAlf(pnt)
			f.write(xcore.packTupF32(pos))
			pk = struct.pack("HHHHHHHH",
				xcore.f16(onrm[0]), xcore.f16(onrm[1]),
				xcore.f16(tex[0]), xcore.f16(1.0 - tex[1]),
				xcore.f16(clr[0]), xcore.f16(clr[1]), xcore.f16(clr[2]), xcore.f16(alf))
			f.write(pk)
			if self.skinData:
				skn = self.skinData[ipnt]
				nwgt = len(skn)
				jidx = [skn[nwgt-1][0] for i in xrange(4)]
				jwgt = [0.0 for i in xrange(4)]
				scl = 0xFFFF
				for i in xrange(nwgt):
					iw = skn[i]
					jidx[i] = iw[0]
					jwgt[i] = max(min(int(round(iw[1] * scl)), scl), 0)
				f.write(struct.pack("HHHH", jwgt[0], jwgt[1], jwgt[2], jwgt[3]))
				f.write(struct.pack("BBBB", jidx[0], jidx[1], jidx[2], jidx[3]))

	def writeShortVB(self, f):
		attrs = MdlPntAttrs(self, self.alfAttrName)
		cscl = 0x7FF
		tscl = 0x7FF # up to 2K
		if self.skinData:
			qscl = 0xFFFF
			scl = [self.bboxMax[i] - self.bboxMin[i] for i in xrange(3)]
			for i in xrange(3):
				if scl[i] != 0.0: scl[i] = float(qscl) / scl[i]
			npnt = self.numPnt()
			for ipnt in xrange(npnt):
				pnt = self.pnts[ipnt]
				pos = self.ppos[ipnt]
				rel = [(pos[i] - self.bboxMin[i]) * scl[i] for i in xrange(3)]
				qpos = [max(min(int(round(rel[i])), qscl), 0) for i in xrange(3)]
				skn = self.skinData[ipnt]
				nwgt = len(skn)
				jidx = [skn[nwgt-1][0] for i in xrange(4)]
				jwgt = [0 for i in xrange(4)]
				for i in xrange(nwgt):
					wscl = 0xFFFF
					iw = skn[i]
					jidx[i] = iw[0]
					jwgt[i] = max(min(int(round(iw[1] * wscl)), wscl), 0)
				f.write(struct.pack("HHHH", qpos[0], qpos[1], qpos[2], jwgt[0]))
				nrm = attrs.getNrm(pnt)
				onrm = xcore.encodeOcta(nrm)
				for i in xrange(2): onrm[i] = (onrm[i] + 1.0) * 0.5
				f.write(struct.pack("HHHH", xcore.u16(onrm[0]), xcore.u16(onrm[1]), jwgt[1], jwgt[2]))
				clr = attrs.getClr(pnt)
				alf = attrs.getAlf(pnt)
				f.write(struct.pack("HHHH", xcore.u16(clr[0], cscl), xcore.u16(clr[1], cscl), xcore.u16(clr[2], cscl), xcore.u16(alf, cscl)))
				tex = attrs.getTex(pnt)
				f.write(struct.pack("hh", xcore.i16(tex[0], tscl), xcore.i16(1.0 - tex[1], tscl)))
				f.write(struct.pack("BBBB", jidx[0], jidx[1], jidx[2], jidx[3]))
		else:
			npnt = self.numPnt()
			for ipnt in xrange(npnt):
				pnt = self.pnts[ipnt]
				pos = self.ppos[ipnt]
				f.write(struct.pack("fff", pos[0], pos[1], pos[2]))
				nrm = attrs.getNrm(pnt)
				onrm = xcore.encodeOcta(nrm)
				f.write(struct.pack("hh", xcore.i16(onrm[0]), xcore.i16(onrm[1])))
				clr = attrs.getClr(pnt)
				alf = attrs.getAlf(pnt)
				f.write(struct.pack("HHHH", xcore.u16(clr[0], cscl), xcore.u16(clr[1], cscl), xcore.u16(clr[2], cscl), xcore.u16(alf, cscl)))
				tex = attrs.getTex(pnt)
				f.write(struct.pack("ff", tex[0], 1.0 - tex[1]))

	def writeData(self, bw, top):
		f = bw.getFile()
		npnt = self.numPnt()
		ntri = self.ntris
		nmtl = self.numMtl()
		nbat = self.numBat()
		nskn = self.numSkin()
		nskl = self.numSkel()
		ntex = len(self.tlst)

		if nskn > 0:
			bw.align(0x10)
			skinInfoTop = bw.getPos()
			bw.patch(self.patchPos + 0x10, skinInfoTop - top) # -> skin
			f.write(struct.pack("I", 0)) # +00 -> mdl spheres
			f.write(struct.pack("I", 0)) # +04 -> bat spheres
			f.write(struct.pack("I", 0)) # +08 -> names
			f.write(struct.pack("I", 0)) # +0C -> skel map
			f.write(struct.pack("I", 0)) # +10 -> bat jlst data
			f.write(struct.pack("I", len(self.jsphBats) // 4)) # +14 -> num bat items (spheres/jlst data)
			bw.align(0x10)
			bw.patch(skinInfoTop, bw.getPos() - top)
			f.write(xcore.packTupF32(self.jsphMdl))
			bw.patch(skinInfoTop + 4, bw.getPos() - top)
			f.write(xcore.packTupF32(self.jsphBats))
			bw.patch(skinInfoTop + 8, bw.getPos() - top)
			for skinName in self.skinNames:
				nameId = self.skinNameToStrId[skinName]
				self.writeStrId32(bw, nameId)
			bw.patch(skinInfoTop + 0xC, bw.getPos() - top)
			for skinName in self.skinNames:
				skelIdx = -1
				if self.skelNodes:
					if skinName in self.skelNodeMap:
						skelIdx = self.skelNodeMap[skinName]
				bw.writeI32(skelIdx)
			bw.patch(skinInfoTop + 0x10, bw.getPos() - top)
			for bat in self.bats:
				for ijnt in bat.ijnts:
					f.write(struct.pack("i", ijnt))

		if nskl > 0:
			bw.align(0x10)
			bw.patch(self.patchPos + 0x14, bw.getPos() - top) # -> skel
			mdata = b""
			for skelNode in self.skelNodes:
				mdata += packXMtx(skelNode.lmtx)
			for skelNode in self.skelNodes:
				mdata += packXMtx(skelNode.imtx)
			f.write(mdata)
			for skelNode in self.skelNodes:
				self.writeStrId32(bw, skelNode.nameId)
			for skelNode in self.skelNodes:
				f.write(struct.pack("i", skelNode.parentId))

		if nmtl > 0:
			bw.align(0x10)
			mtlsTop = bw.getPos()
			bw.patch(self.patchPos + 4, mtlsTop - top) # -> mtls
			for mtl in self.mtls:
				mtl.write(bw)
			if self.swpMtls and len(self.swpMtls) > 0:
				for mtl in self.swpMtls:
					mtl.write(bw)
			for imtl, mtl in enumerate(self.mtls):
				if mtl.swapIds:
					bw.patch(mtlsTop + (imtl*MdlMaterial.SIZE) + MdlMaterial.SWAPS_OFFS, bw.getPos() - top)
					bw.writeI32(len(mtl.swapIds))
					for swpId in mtl.swapIds:
						bw.writeI32(swpId)

		if ntex > 0:
			bw.align(0x10)
			bw.patch(self.patchPos + 8, bw.getPos() - top) # -> texs
			for tex in self.tlst:
				self.writeStrId32(bw, tex[0]) # nameId
				self.writeStrId32(bw, tex[1]) # pathId
				bw.writeU32(0) # work[0]
				bw.writeU32(0) # work[1]

		if nbat > 0:
			bw.align(0x10)
			bw.patch(self.patchPos + 0xC, bw.getPos() - top) # -> bats
			for bat in self.bats: # bbox[]
				f.write(xcore.packTupF32(bat.bboxMin + bat.bboxMax))
			for bat in self.bats: # info[]
				bat.write(bw)

		bw.align(0x10)
		bw.patch(self.patchPos + 0, bw.getPos() - top) # -> pnts
		if self.useHalf:
			self.writeHalfVB(f)
		else:
			self.writeShortVB(f)

		if self.nidx16 > 0:
			bw.align(0x10)
			bw.patch(self.patchPos + 0x18, bw.getPos() - top) # -> idx16
			for bat in self.bats:
				if bat.isIdx16():
					grp = self.grps[bat.igrp]
					for i in xrange(bat.npol):
						ipol = grp.ipols[bat.org + i]
						pol = self.pols[ipol]
						for idx in pol.ibuf:
							rel = (idx - bat.minIdx) & 0xFFFF
							f.write(struct.pack("H", rel))

		if self.nidx32 > 0:
			bw.align(0x10)
			bw.patch(self.patchPos + 0x1C, bw.getPos() - top) # -> idx32
			for bat in self.bats:
				if not bat.isIdx16():
					grp = self.grps[bat.igrp]
					for i in xrange(bat.npol):
						ipol = grp.ipols[bat.org + i]
						pol = self.pols[ipol]
						for idx in pol.ibuf:
							rel = idx - bat.minIdx
							f.write(struct.pack("I", rel))

	def save(self, outPath):
		if self.isStatic: self.flags |= 1
		if self.useHalf: self.flags |= 2
		xcore.BaseExporter.save(self, outPath)
