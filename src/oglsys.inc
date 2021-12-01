#if defined(OGL_FN_CORE) && defined(OGL_FN)
OGL_FN(VIEWPORT, Viewport)
OGL_FN(SCISSOR, Scissor)
OGL_FN(GETINTEGERV, GetIntegerv)
OGL_FN(GETSTRING, GetString)
OGL_FN(LINEWIDTH, LineWidth)
OGL_FN(COLORMASK, ColorMask)
OGL_FN(DEPTHMASK, DepthMask)
OGL_FN(DEPTHFUNC, DepthFunc)
OGL_FN(STENCILMASK, StencilMask)
OGL_FN(STENCILOP, StencilOp)
OGL_FN(STENCILFUNC, StencilFunc)
OGL_FN(STENCILMASKSEPARATE, StencilMaskSeparate)
OGL_FN(STENCILOPSEPARATE, StencilOpSeparate)
OGL_FN(STENCILFUNCSEPARATE, StencilFuncSeparate)
OGL_FN(CLEARCOLOR, ClearColor)
OGL_FN(CLEARDEPTH, ClearDepth)
OGL_FN(CLEARSTENCIL, ClearStencil)
OGL_FN(CLEAR, Clear)
OGL_FN(ENABLE, Enable)
OGL_FN(DISABLE, Disable)
OGL_FN(FRONTFACE, FrontFace)
OGL_FN(CULLFACE, CullFace)
OGL_FN(BLENDEQUATION, BlendEquation)
OGL_FN(BLENDFUNC, BlendFunc)
OGL_FN(BLENDEQUATIONSEPARATE, BlendEquationSeparate)
OGL_FN(BLENDFUNCSEPARATE, BlendFuncSeparate)
OGL_FN(GENBUFFERS, GenBuffers)
OGL_FN(BINDBUFFER, BindBuffer)
OGL_FN(BUFFERDATA, BufferData)
OGL_FN(BUFFERSUBDATA, BufferSubData)
OGL_FN(BINDBUFFERRANGE, BindBufferRange)
OGL_FN(DELETEBUFFERS, DeleteBuffers)
OGL_FN(GENSAMPLERS, GenSamplers)
OGL_FN(DELETESAMPLERS, DeleteSamplers)
OGL_FN(SAMPLERPARAMETERI, SamplerParameteri)
OGL_FN(BINDSAMPLER, BindSampler)
OGL_FN(GENTEXTURES, GenTextures)
OGL_FN(DELETETEXTURES, DeleteTextures)
OGL_FN(BINDTEXTURE, BindTexture)
OGL_FN(TEXIMAGE2D, TexImage2D)
OGL_FN(TEXSTORAGE2D, TexStorage2D)
OGL_FN(TEXSTORAGE3D, TexStorage3D)
OGL_FN(TEXSUBIMAGE3D, TexSubImage3D)
OGL_FN(TEXPARAMETERI, TexParameteri)
OGL_FN(ACTIVETEXTURE, ActiveTexture)
OGL_FN(GENERATEMIPMAP, GenerateMipmap)
OGL_FN(PIXELSTOREI, PixelStorei)
OGL_FN(CREATESHADER, CreateShader)
OGL_FN(SHADERSOURCE, ShaderSource)
OGL_FN(COMPILESHADER, CompileShader)
OGL_FN(DELETESHADER, DeleteShader)
OGL_FN(GETSHADERIV, GetShaderiv)
OGL_FN(GETSHADERINFOLOG, GetShaderInfoLog)
OGL_FN(ATTACHSHADER, AttachShader)
OGL_FN(DETACHSHADER, DetachShader)
OGL_FN(CREATEPROGRAM, CreateProgram)
OGL_FN(LINKPROGRAM, LinkProgram)
OGL_FN(DELETEPROGRAM, DeleteProgram)
OGL_FN(USEPROGRAM, UseProgram)
OGL_FN(GETPROGRAMIV, GetProgramiv)
OGL_FN(GETPROGRAMINFOLOG, GetProgramInfoLog)
OGL_FN(GETUNIFORMLOCATION, GetUniformLocation)
OGL_FN(UNIFORM1I, Uniform1i)
OGL_FN(UNIFORM1F, Uniform1f)
OGL_FN(UNIFORM1FV, Uniform1fv)
OGL_FN(UNIFORM2FV, Uniform2fv)
OGL_FN(UNIFORM3FV, Uniform3fv)
OGL_FN(UNIFORM4FV, Uniform4fv)
OGL_FN(UNIFORMMATRIX4FV, UniformMatrix4fv)
OGL_FN(GETATTRIBLOCATION, GetAttribLocation)
OGL_FN(GETUNIFORMBLOCKINDEX, GetUniformBlockIndex)
OGL_FN(UNIFORMBLOCKBINDING, UniformBlockBinding)
OGL_FN(VERTEXATTRIBPOINTER, VertexAttribPointer)
OGL_FN(ENABLEVERTEXATTRIBARRAY, EnableVertexAttribArray)
OGL_FN(DISABLEVERTEXATTRIBARRAY, DisableVertexAttribArray)
OGL_FN(VERTEXATTRIB4FV, VertexAttrib4fv)
OGL_FN(VERTEXATTRIB4IV, VertexAttrib4iv)
OGL_FN(BINDBUFFERBASE, BindBufferBase)
OGL_FN(DRAWARRAYS, DrawArrays)
OGL_FN(DRAWELEMENTS, DrawElements)
OGL_FN(GENFRAMEBUFFERS, GenFramebuffers)
OGL_FN(DELETEFRAMEBUFFERS, DeleteFramebuffers)
OGL_FN(BINDFRAMEBUFFER, BindFramebuffer)
OGL_FN(FRAMEBUFFERTEXTURE2D, FramebufferTexture2D)
OGL_FN(FRAMEBUFFERRENDERBUFFER, FramebufferRenderbuffer)
OGL_FN(CHECKFRAMEBUFFERSTATUS, CheckFramebufferStatus)
OGL_FN(BLITFRAMEBUFFER, BlitFramebuffer)
OGL_FN(GENRENDERBUFFERS, GenRenderbuffers)
OGL_FN(DELETERENDERBUFFERS, DeleteRenderbuffers)
OGL_FN(BINDRENDERBUFFER, BindRenderbuffer)
OGL_FN(RENDERBUFFERSTORAGE, RenderbufferStorage)
OGL_FN(GETERROR, GetError)
#endif /* OGL_FN_CORE */

#if defined(OGL_FN_EXTRA) && defined(OGL_FN)
OGL_FN(GETACTIVEUNIFORMBLOCKIV, GetActiveUniformBlockiv)
OGL_FN(GETUNIFORMINDICES, GetUniformIndices)
OGL_FN(GETACTIVEUNIFORMSIV, GetActiveUniformsiv)
OGL_FN(MAPBUFFERRANGE, MapBufferRange)
OGL_FN(UNMAPBUFFER, UnmapBuffer)
OGL_FN(DRAWBUFFER, DrawBuffer)
OGL_FN(DRAWBUFFERS, DrawBuffers)
OGL_FN(CLEARBUFFERFI, ClearBufferfi)
OGL_FN(CLEARBUFFERFV, ClearBufferfv)
OGL_FN(SHADERBINARY, ShaderBinary)
OGL_FN(GENVERTEXARRAYS, GenVertexArrays)
OGL_FN(DELETEVERTEXARRAYS, DeleteVertexArrays)
OGL_FN(BINDVERTEXARRAY, BindVertexArray)
OGL_FN(DRAWELEMENTSBASEVERTEX, DrawElementsBaseVertex)
OGL_FN(VERTEXATTRIBDIVISOR, VertexAttribDivisor)
OGL_FN(SPECIALIZESHADER, SpecializeShader)
OGL_FN(PROGRAMPARAMETERI, ProgramParameteri)
OGL_FN(PROGRAMBINARY, ProgramBinary)
OGL_FN(GETPROGRAMBINARY, GetProgramBinary)
OGL_FN(GETPROGRAMINTERFACEIV, GetProgramInterfaceiv)
OGL_FN(GETPROGRAMRESOURCEIV, GetProgramResourceiv)
OGL_FN(GETPROGRAMRESOURCEINDEX, GetProgramResourceIndex)
OGL_FN(GETPROGRAMRESOURCELOCATION, GetProgramResourceLocation)
OGL_FN(GENQUERIES, GenQueries)
OGL_FN(DELETEQUERIES, DeleteQueries)
OGL_FN(QUERYCOUNTER, QueryCounter)
OGL_FN(GETQUERYOBJECTIV, GetQueryObjectiv)
OGL_FN(GETQUERYOBJECTUIV, GetQueryObjectuiv)
OGL_FN(GETQUERYOBJECTUI64V, GetQueryObjectui64v)
OGL_FN(COMPRESSEDTEXIMAGE2D, CompressedTexImage2D)
OGL_FN(DISPATCHCOMPUTE, DispatchCompute)
OGL_FN(MEMORYBARRIER, MemoryBarrier)
OGL_FN(PATCHPARAMETERI, PatchParameteri)
OGL_FN(PATCHPARAMETERFV, PatchParameterfv)
#endif /* OGL_FN_EXTRA */


#if OGLSYS_CL && defined(OGLSYS_CL_FN)

OGLSYS_CL_FN(GetPlatformIDs)
OGLSYS_CL_FN(GetPlatformInfo)
OGLSYS_CL_FN(GetDeviceIDs)
OGLSYS_CL_FN(GetDeviceInfo)

OGLSYS_CL_FN(CreateContext)
OGLSYS_CL_FN(CreateContextFromType)
OGLSYS_CL_FN(RetainContext)
OGLSYS_CL_FN(ReleaseContext)
OGLSYS_CL_FN(GetContextInfo)

OGLSYS_CL_FN(CreateCommandQueue)
OGLSYS_CL_FN(RetainCommandQueue)
OGLSYS_CL_FN(ReleaseCommandQueue)
OGLSYS_CL_FN(GetCommandQueueInfo)

OGLSYS_CL_FN(CreateBuffer)
OGLSYS_CL_FN(CreateImage)
OGLSYS_CL_FN(RetainMemObject)
OGLSYS_CL_FN(ReleaseMemObject)
OGLSYS_CL_FN(GetSupportedImageFormats)
OGLSYS_CL_FN(GetMemObjectInfo)
OGLSYS_CL_FN(GetImageInfo)

OGLSYS_CL_FN(CreateSampler)
OGLSYS_CL_FN(RetainSampler)
OGLSYS_CL_FN(ReleaseSampler)
OGLSYS_CL_FN(GetSamplerInfo)
OGLSYS_CL_FN(CreateProgramWithSource)
OGLSYS_CL_FN(CreateProgramWithBinary)
OGLSYS_CL_FN(CreateProgramWithBuiltInKernels)
OGLSYS_CL_FN(RetainProgram)
OGLSYS_CL_FN(ReleaseProgram)
OGLSYS_CL_FN(BuildProgram)
OGLSYS_CL_FN(CompileProgram)
OGLSYS_CL_FN(LinkProgram)
OGLSYS_CL_FN(GetProgramInfo)
OGLSYS_CL_FN(GetProgramBuildInfo)

OGLSYS_CL_FN(CreateKernel)
OGLSYS_CL_FN(CreateKernelsInProgram)
OGLSYS_CL_FN(RetainKernel)
OGLSYS_CL_FN(ReleaseKernel)
OGLSYS_CL_FN(SetKernelArg)
OGLSYS_CL_FN(GetKernelInfo)
OGLSYS_CL_FN(GetKernelArgInfo)
OGLSYS_CL_FN(GetKernelWorkGroupInfo)

OGLSYS_CL_FN(WaitForEvents)
OGLSYS_CL_FN(GetEventInfo)
OGLSYS_CL_FN(RetainEvent)
OGLSYS_CL_FN(ReleaseEvent)
OGLSYS_CL_FN(GetEventProfilingInfo)

OGLSYS_CL_FN(Flush)
OGLSYS_CL_FN(Finish)

OGLSYS_CL_FN(EnqueueReadBuffer)
OGLSYS_CL_FN(EnqueueReadBufferRect)
OGLSYS_CL_FN(EnqueueWriteBuffer)
OGLSYS_CL_FN(EnqueueWriteBufferRect)
OGLSYS_CL_FN(EnqueueFillBuffer)
OGLSYS_CL_FN(EnqueueCopyBuffer)
OGLSYS_CL_FN(EnqueueCopyBufferRect)
OGLSYS_CL_FN(EnqueueReadImage)
OGLSYS_CL_FN(EnqueueWriteImage)
OGLSYS_CL_FN(EnqueueFillImage)
OGLSYS_CL_FN(EnqueueCopyImage)
OGLSYS_CL_FN(EnqueueCopyImageToBuffer)
OGLSYS_CL_FN(EnqueueCopyBufferToImage)
OGLSYS_CL_FN(EnqueueMapBuffer)
OGLSYS_CL_FN(EnqueueMapImage)
OGLSYS_CL_FN(EnqueueUnmapMemObject)
OGLSYS_CL_FN(EnqueueMigrateMemObjects)
OGLSYS_CL_FN(EnqueueNDRangeKernel)
OGLSYS_CL_FN(EnqueueTask)
OGLSYS_CL_FN(EnqueueNativeKernel)
OGLSYS_CL_FN(EnqueueMarkerWithWaitList)
OGLSYS_CL_FN(EnqueueBarrierWithWaitList)

OGLSYS_CL_FN(GetExtensionFunctionAddressForPlatform)

#endif /* OGLSYS_CL */
