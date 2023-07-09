@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
call ..\..\gpu_paths.bat

set DST_DIR=..\..\bin\data\ogl
if not exist %DST_DIR% mkdir %DST_DIR%

set VTX_TMP=vtx_tmp.glsl
set SHADERS=shaders.inc

goto :start

:make_vtx_descr
	echo /* %1 */ > %VTX_TMP%
	for /f "delims=!" %%i in (%1.txt) do (
		echo attribute %%i; >> %VTX_TMP%
	)
goto :EOF

:vert
	call :make_vtx_descr %1
	set dst=%DST_DIR%\%2
	set src=prologue_vert.h+gpu_defs.h+gpu_params_vert.h+%VTX_TMP%+frag.h+f_vtxout.glsl+f_octa.glsl+%3
	echo %2
	echo GPU_SHADER(%~n2, vert) >> %SHADERS%
	copy /BY %src% %dst% > nul
	if not x%GLSL%==x (%GLSL% %dst%)
goto :EOF

:frag
	set dst=%DST_DIR%\%1
	set top=prologue_frag.h
	if not x%3==x (
		set top=%3
	)
	set src=%top%+gpu_defs.h+gpu_params_frag.h+frag.h+texs.h+f_tex.glsl+f_pixout.glsl+%2
	echo %1
	echo GPU_SHADER(%~n1, frag) >> %SHADERS%
	copy /BY %src% %dst% > nul
	if not x%GLSL%==x (%GLSL% %dst%)
goto :EOF

:start
echo /* */ > %SHADERS%


call :vert vtx_skin0 skin0.vert f_skin.glsl+skin0.vert
call :vert vtx_rigid0 rigid0.vert rigid0.vert

call :vert vtx_skin1 skin1.vert f_skin.glsl+skin1.vert
call :vert vtx_rigid1 rigid1.vert rigid1.vert


rem vertex hemi lighting

call :vert vtx_skin0 skin0_vl.vert f_skin.glsl+f_hemi.glsl+skin0_vl.vert
call :vert vtx_rigid0 rigid0_vl.vert f_hemi.glsl+rigid0_vl.vert

call :vert vtx_skin1 skin1_vl.vert f_skin.glsl+f_hemi.glsl+skin1_vl.vert
call :vert vtx_rigid1 rigid1_vl.vert f_hemi.glsl+rigid1_vl.vert



rem hemi light

call :frag hemi_opaq.frag f_hemi.glsl+hemi_basic.glsl+hemi_opaq.frag
call :frag hemi_semi.frag f_hemi.glsl+hemi_basic.glsl+hemi_semi.frag
call :frag hemi_limit.frag f_hemi.glsl+hemi_basic.glsl+hemi_limit.frag
call :frag hemi_discard.frag f_hemi.glsl+hemi_basic.glsl+hemi_discard.frag

call :frag hemi_opaq_sdw.frag f_shadow.glsl+f_hemi.glsl+hemi_basic_sdw.glsl+hemi_opaq_sdw.frag
call :frag hemi_semi_sdw.frag f_shadow.glsl+f_hemi.glsl+hemi_basic_sdw.glsl+hemi_semi_sdw.frag
call :frag hemi_limit_sdw.frag f_shadow.glsl+f_hemi.glsl+hemi_basic_sdw.glsl+hemi_limit_sdw.frag
call :frag hemi_discard_sdw.frag f_shadow.glsl+f_hemi.glsl+hemi_basic_sdw.glsl+hemi_discard_sdw.frag


call :frag hemi_spec_opaq.frag f_hemi.glsl+f_spec.glsl+hemi_spec_basic.glsl+hemi_spec_opaq.frag
call :frag hemi_spec_semi.frag f_hemi.glsl+f_spec.glsl+hemi_spec_basic.glsl+hemi_spec_semi.frag
call :frag hemi_spec_limit.frag f_hemi.glsl+f_spec.glsl+hemi_spec_basic.glsl+hemi_spec_limit.frag
call :frag hemi_spec_discard.frag f_hemi.glsl+f_spec.glsl+hemi_spec_basic.glsl+hemi_spec_discard.frag

call :frag hemi_spec_opaq_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+hemi_spec_basic_sdw.glsl+hemi_spec_opaq_sdw.frag
call :frag hemi_spec_semi_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+hemi_spec_basic_sdw.glsl+hemi_spec_semi_sdw.frag
call :frag hemi_spec_limit_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+hemi_spec_basic_sdw.glsl+hemi_spec_limit_sdw.frag
call :frag hemi_spec_discard_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+hemi_spec_basic_sdw.glsl+hemi_spec_discard_sdw.frag


call :frag hemi_bump_opaq.frag f_hemi.glsl+f_bump.glsl+hemi_bump.glsl+hemi_bump_opaq.frag prologue_frag_bump.h
call :frag hemi_bump_semi.frag f_hemi.glsl+f_bump.glsl+hemi_bump.glsl+hemi_bump_semi.frag prologue_frag_bump.h
call :frag hemi_bump_limit.frag f_hemi.glsl+f_bump.glsl+hemi_bump.glsl+hemi_bump_limit.frag prologue_frag_bump.h
call :frag hemi_bump_discard.frag f_hemi.glsl+f_bump.glsl+hemi_bump.glsl+hemi_bump_discard.frag prologue_frag_bump.h

call :frag hemi_bump_opaq_sdw.frag f_shadow.glsl+f_hemi.glsl+f_bump.glsl+hemi_bump_sdw.glsl+hemi_bump_opaq_sdw.frag prologue_frag_bump.h
call :frag hemi_bump_semi_sdw.frag f_shadow.glsl+f_hemi.glsl+f_bump.glsl+hemi_bump_sdw.glsl+hemi_bump_semi_sdw.frag prologue_frag_bump.h
call :frag hemi_bump_limit_sdw.frag f_shadow.glsl+f_hemi.glsl+f_bump.glsl+hemi_bump_sdw.glsl+hemi_bump_limit_sdw.frag prologue_frag_bump.h
call :frag hemi_bump_discard_sdw.frag f_shadow.glsl+f_hemi.glsl+f_bump.glsl+hemi_bump_sdw.glsl+hemi_bump_discard_sdw.frag prologue_frag_bump.h


call :frag hemi_spec_bump_opaq.frag f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump.glsl+hemi_spec_bump_opaq.frag prologue_frag_bump.h
call :frag hemi_spec_bump_semi.frag f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump.glsl+hemi_spec_bump_semi.frag prologue_frag_bump.h
call :frag hemi_spec_bump_limit.frag f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump.glsl+hemi_spec_bump_limit.frag prologue_frag_bump.h
call :frag hemi_spec_bump_discard.frag f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump.glsl+hemi_spec_bump_discard.frag prologue_frag_bump.h

call :frag hemi_spec_bump_opaq_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_sdw.glsl+hemi_spec_bump_opaq_sdw.frag prologue_frag_bump.h
call :frag hemi_spec_bump_semi_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_sdw.glsl+hemi_spec_bump_semi_sdw.frag prologue_frag_bump.h
call :frag hemi_spec_bump_limit_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_sdw.glsl+hemi_spec_bump_limit_sdw.frag prologue_frag_bump.h
call :frag hemi_spec_bump_discard_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_sdw.glsl+hemi_spec_bump_discard_sdw.frag prologue_frag_bump.h

rem no semi-transparent mode for patterned bump
call :frag hemi_bump_pat_opaq.frag f_hemi.glsl+f_bump.glsl+hemi_bump_pat.glsl+hemi_bump_pat_opaq.frag prologue_frag_bump.h
call :frag hemi_bump_pat_limit.frag f_hemi.glsl+f_bump.glsl+hemi_bump_pat.glsl+hemi_bump_pat_limit.frag prologue_frag_bump.h
call :frag hemi_bump_pat_discard.frag f_hemi.glsl+f_bump.glsl+hemi_bump_pat.glsl+hemi_bump_pat_discard.frag prologue_frag_bump.h

call :frag hemi_bump_pat_opaq_sdw.frag f_shadow.glsl+f_hemi.glsl+f_bump.glsl+hemi_bump_pat_sdw.glsl+hemi_bump_pat_opaq_sdw.frag prologue_frag_bump.h
call :frag hemi_bump_pat_limit_sdw.frag f_shadow.glsl+f_hemi.glsl+f_bump.glsl+hemi_bump_pat_sdw.glsl+hemi_bump_pat_limit_sdw.frag prologue_frag_bump.h
call :frag hemi_bump_pat_discard_sdw.frag f_shadow.glsl+f_hemi.glsl+f_bump.glsl+hemi_bump_pat_sdw.glsl+hemi_bump_pat_discard_sdw.frag prologue_frag_bump.h

rem spec bump pat, no semi
call :frag hemi_spec_bump_pat_opaq.frag f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_pat.glsl+hemi_spec_bump_pat_opaq.frag prologue_frag_bump.h
call :frag hemi_spec_bump_pat_limit.frag f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_pat.glsl+hemi_spec_bump_pat_limit.frag prologue_frag_bump.h
call :frag hemi_spec_bump_pat_discard.frag f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_pat.glsl+hemi_spec_bump_pat_discard.frag prologue_frag_bump.h

call :frag hemi_spec_bump_pat_opaq_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_pat_sdw.glsl+hemi_spec_bump_pat_opaq_sdw.frag prologue_frag_bump.h
call :frag hemi_spec_bump_pat_limit_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_pat_sdw.glsl+hemi_spec_bump_pat_limit_sdw.frag prologue_frag_bump.h
call :frag hemi_spec_bump_pat_discard_sdw.frag f_shadow.glsl+f_hemi.glsl+f_spec.glsl+f_bump.glsl+hemi_spec_bump_pat_sdw.glsl+hemi_spec_bump_pat_discard_sdw.frag prologue_frag_bump.h


rem SH3 lighting

call :frag sh_opaq.frag f_sh.glsl+sh_basic.glsl+sh_opaq.frag
call :frag sh_semi.frag f_sh.glsl+sh_basic.glsl+sh_semi.frag
call :frag sh_limit.frag f_sh.glsl+sh_basic.glsl+sh_limit.frag
call :frag sh_discard.frag f_sh.glsl+sh_basic.glsl+sh_discard.frag

call :frag sh_opaq_sdw.frag f_shadow.glsl+f_sh.glsl+sh_basic_sdw.glsl+sh_opaq_sdw.frag
call :frag sh_semi_sdw.frag f_shadow.glsl+f_sh.glsl+sh_basic_sdw.glsl+sh_semi_sdw.frag
call :frag sh_limit_sdw.frag f_shadow.glsl+f_sh.glsl+sh_basic_sdw.glsl+sh_limit_sdw.frag
call :frag sh_discard_sdw.frag f_shadow.glsl+f_sh.glsl+sh_basic_sdw.glsl+sh_discard_sdw.frag


rem no lighting

call :frag unlit_opaq.frag unlit.glsl+unlit_opaq.frag
call :frag unlit_semi.frag unlit.glsl+unlit_semi.frag
call :frag unlit_limit.frag unlit.glsl+unlit_limit.frag
call :frag unlit_discard.frag unlit.glsl+unlit_discard.frag

call :frag unlit_opaq_sdw.frag f_shadow.glsl+unlit_sdw.glsl+unlit_opaq_sdw.frag
call :frag unlit_semi_sdw.frag f_shadow.glsl+unlit_sdw.glsl+unlit_semi_sdw.frag
call :frag unlit_limit_sdw.frag f_shadow.glsl+unlit_sdw.glsl+unlit_limit_sdw.frag
call :frag unlit_discard_sdw.frag f_shadow.glsl+unlit_sdw.glsl+unlit_discard_sdw.frag



rem shadow cast
call :frag cast_opaq.frag f_shadow.glsl+cast_opaq.frag
call :frag cast_semi.frag f_shadow.glsl+cast_semi.frag


rem prims
call :vert vtx_prim prim.vert f_hemi.glsl+prim.vert
call :frag prim.frag prim.frag


rem screen
call :vert vtx_quad quad.vert quad.vert
call :frag quad.frag quad.frag

rem font
call :vert vtx_font font.vert font.vert
call :frag font.frag font.frag
