##########################################################################
#
# Copyright 2012 Jose Fonseca
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
##########################################################################/

"""d3d11_1.h"""


from winapi import *
from d3d11sdklayers import *
from d3d11 import *

D3D_FEATURE_LEVEL.values += [
    "D3D_FEATURE_LEVEL_11_1",
]

D3D11_CREATE_DEVICE_FLAG.values += [
    "D3D11_CREATE_DEVICE_DEBUGGABLE",
    "D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY",
    "D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT",
    "D3D11_CREATE_DEVICE_VIDEO_SUPPORT",
]

D3D_MIN_PRECISION = Enum("D3D_MIN_PRECISION", [
    "D3D_MIN_PRECISION_DEFAULT",
    "D3D_MIN_PRECISION_FLOAT_16",
    "D3D_MIN_PRECISION_FLOAT_2_8",
    "D3D_MIN_PRECISION_RESERVED",
    "D3D_MIN_PRECISION_SINT_16",
    "D3D_MIN_PRECISION_UINT_16",
    "D3D_MIN_PRECISION_ANY_16",
    "D3D_MIN_PRECISION_ANY_10",
])

ID3D11BlendState1 = Interface("ID3D11BlendState1", ID3D11BlendState)
ID3D11RasterizerState1 = Interface("ID3D11RasterizerState1", ID3D11RasterizerState)
ID3DDeviceContextState = Interface("ID3DDeviceContextState", ID3D11DeviceChild)
ID3D11DeviceContext1 = Interface("ID3D11DeviceContext1", ID3D11DeviceContext)
ID3D11Device1 = Interface("ID3D11Device1", ID3D11Device)
ID3DUserDefinedAnnotation = Interface("ID3DUserDefinedAnnotation", IUnknown)


D3D11_COPY_FLAGS = Flags(UINT, [
    "D3D11_COPY_NO_OVERWRITE",
    "D3D11_COPY_DISCARD",
])

D3D11_LOGIC_OP = Enum("D3D11_LOGIC_OP", [
    "D3D11_LOGIC_OP_CLEAR",
    "D3D11_LOGIC_OP_SET",
    "D3D11_LOGIC_OP_COPY",
    "D3D11_LOGIC_OP_COPY_INVERTED",
    "D3D11_LOGIC_OP_NOOP",
    "D3D11_LOGIC_OP_INVERT",
    "D3D11_LOGIC_OP_AND",
    "D3D11_LOGIC_OP_NAND",
    "D3D11_LOGIC_OP_OR",
    "D3D11_LOGIC_OP_NOR",
    "D3D11_LOGIC_OP_XOR",
    "D3D11_LOGIC_OP_EQUIV",
    "D3D11_LOGIC_OP_AND_REVERSE",
    "D3D11_LOGIC_OP_AND_INVERTED",
    "D3D11_LOGIC_OP_OR_REVERSE",
    "D3D11_LOGIC_OP_OR_INVERTED",
])

D3D11_RENDER_TARGET_BLEND_DESC1 = Struct("D3D11_RENDER_TARGET_BLEND_DESC1", [
    (BOOL, "BlendEnable"),
    (BOOL, "LogicOpEnable"),
    (D3D11_BLEND, "SrcBlend"),
    (D3D11_BLEND, "DestBlend"),
    (D3D11_BLEND_OP, "BlendOp"),
    (D3D11_BLEND, "SrcBlendAlpha"),
    (D3D11_BLEND, "DestBlendAlpha"),
    (D3D11_BLEND_OP, "BlendOpAlpha"),
    (D3D11_LOGIC_OP, "LogicOp"),
    (UINT8, "RenderTargetWriteMask"),
])

D3D11_BLEND_DESC1 = Struct("D3D11_BLEND_DESC1", [
    (BOOL, "AlphaToCoverageEnable"),
    (BOOL, "IndependentBlendEnable"),
    (Array(D3D11_RENDER_TARGET_BLEND_DESC1, "D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT"), "RenderTarget"),
])

ID3D11BlendState1.methods += [
    Method(Void, "GetDesc1", [Out(Pointer(D3D11_BLEND_DESC1), "pDesc")]),
]

D3D11_RASTERIZER_DESC1 = Struct("D3D11_RASTERIZER_DESC1", [
    (D3D11_FILL_MODE, "FillMode"),
    (D3D11_CULL_MODE, "CullMode"),
    (BOOL, "FrontCounterClockwise"),
    (INT, "DepthBias"),
    (FLOAT, "DepthBiasClamp"),
    (FLOAT, "SlopeScaledDepthBias"),
    (BOOL, "DepthClipEnable"),
    (BOOL, "ScissorEnable"),
    (BOOL, "MultisampleEnable"),
    (BOOL, "AntialiasedLineEnable"),
    (UINT, "ForcedSampleCount"),
])

ID3D11RasterizerState1.methods += [
    Method(Void, "GetDesc1", [Out(Pointer(D3D11_RASTERIZER_DESC1), "pDesc")]),
]

D3D11_1_CREATE_DEVICE_CONTEXT_STATE_FLAG = Flags(UINT, [
    "D3D11_1_CREATE_DEVICE_CONTEXT_STATE_SINGLETHREADED",
])

ID3D11DeviceContext1.methods += [
    Method(Void, "CopySubresourceRegion1", [(ObjPointer(ID3D11Resource), "pDstResource"), (UINT, "DstSubresource"), (UINT, "DstX"), (UINT, "DstY"), (UINT, "DstZ"), (ObjPointer(ID3D11Resource), "pSrcResource"), (UINT, "SrcSubresource"), (Pointer(Const(D3D11_BOX)), "pSrcBox"), (D3D11_COPY_FLAGS, "CopyFlags")]),
    Method(Void, "UpdateSubresource1", [(ObjPointer(ID3D11Resource), "pDstResource"), (UINT, "DstSubresource"), (Pointer(Const(D3D11_BOX)), "pDstBox"), (OpaquePointer(Const(Void)), "pSrcData"), (UINT, "SrcRowPitch"), (UINT, "SrcDepthPitch"), (D3D11_COPY_FLAGS, "CopyFlags")]),
    Method(Void, "DiscardResource", [(ObjPointer(ID3D11Resource), "pResource")]),
    Method(Void, "DiscardView", [(ObjPointer(ID3D11View), "pResourceView")]),
    Method(Void, "VSSetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), (Array(Const(ObjPointer(ID3D11Buffer)), "NumBuffers"), "ppConstantBuffers"), (Array(Const(UINT), "NumBuffers"), "pFirstConstant"), (Array(Const(UINT), "NumBuffers"), "pNumConstants")]),
    Method(Void, "HSSetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), (Array(Const(ObjPointer(ID3D11Buffer)), "NumBuffers"), "ppConstantBuffers"), (Array(Const(UINT), "NumBuffers"), "pFirstConstant"), (Array(Const(UINT), "NumBuffers"), "pNumConstants")]),
    Method(Void, "DSSetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), (Array(Const(ObjPointer(ID3D11Buffer)), "NumBuffers"), "ppConstantBuffers"), (Array(Const(UINT), "NumBuffers"), "pFirstConstant"), (Array(Const(UINT), "NumBuffers"), "pNumConstants")]),
    Method(Void, "GSSetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), (Array(Const(ObjPointer(ID3D11Buffer)), "NumBuffers"), "ppConstantBuffers"), (Array(Const(UINT), "NumBuffers"), "pFirstConstant"), (Array(Const(UINT), "NumBuffers"), "pNumConstants")]),
    Method(Void, "PSSetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), (Array(Const(ObjPointer(ID3D11Buffer)), "NumBuffers"), "ppConstantBuffers"), (Array(Const(UINT), "NumBuffers"), "pFirstConstant"), (Array(Const(UINT), "NumBuffers"), "pNumConstants")]),
    Method(Void, "CSSetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), (Array(Const(ObjPointer(ID3D11Buffer)), "NumBuffers"), "ppConstantBuffers"), (Array(Const(UINT), "NumBuffers"), "pFirstConstant"), (Array(Const(UINT), "NumBuffers"), "pNumConstants")]),
    Method(Void, "VSGetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), Out(Array(ObjPointer(ID3D11Buffer), "NumBuffers"), "ppConstantBuffers"), Out(Array(UINT, "NumBuffers"), "pFirstConstant"), Out(Array(UINT, "NumBuffers"), "pNumConstants")]),
    Method(Void, "HSGetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), Out(Array(ObjPointer(ID3D11Buffer), "NumBuffers"), "ppConstantBuffers"), Out(Array(UINT, "NumBuffers"), "pFirstConstant"), Out(Array(UINT, "NumBuffers"), "pNumConstants")]),
    Method(Void, "DSGetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), Out(Array(ObjPointer(ID3D11Buffer), "NumBuffers"), "ppConstantBuffers"), Out(Array(UINT, "NumBuffers"), "pFirstConstant"), Out(Array(UINT, "NumBuffers"), "pNumConstants")]),
    Method(Void, "GSGetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), Out(Array(ObjPointer(ID3D11Buffer), "NumBuffers"), "ppConstantBuffers"), Out(Array(UINT, "NumBuffers"), "pFirstConstant"), Out(Array(UINT, "NumBuffers"), "pNumConstants")]),
    Method(Void, "PSGetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), Out(Array(ObjPointer(ID3D11Buffer), "NumBuffers"), "ppConstantBuffers"), Out(Array(UINT, "NumBuffers"), "pFirstConstant"), Out(Array(UINT, "NumBuffers"), "pNumConstants")]),
    Method(Void, "CSGetConstantBuffers1", [(UINT, "StartSlot"), (UINT, "NumBuffers"), Out(Array(ObjPointer(ID3D11Buffer), "NumBuffers"), "ppConstantBuffers"), Out(Array(UINT, "NumBuffers"), "pFirstConstant"), Out(Array(UINT, "NumBuffers"), "pNumConstants")]),
    Method(Void, "SwapDeviceContextState", [(ObjPointer(ID3DDeviceContextState), "pState"), Out(Pointer(ObjPointer(ID3DDeviceContextState)), "ppPreviousState")]),
    Method(Void, "ClearView", [(ObjPointer(ID3D11View), "pView"), (Array(Const(FLOAT), 4), "Color"), (Pointer(Const(D3D11_RECT)), "pRect"), (UINT, "NumRects")]),
]


DXGI_SHARED_RESOURCE_FLAG = Flags(DWORD, [
    "DXGI_SHARED_RESOURCE_READ",
    "DXGI_SHARED_RESOURCE_WRITE",
])


ID3D11Device1.methods += [
    Method(Void, "GetImmediateContext1", [Out(Pointer(ObjPointer(ID3D11DeviceContext1)), "ppImmediateContext")]),
    Method(HRESULT, "CreateDeferredContext1", [(UINT, "ContextFlags"), Out(Pointer(ObjPointer(ID3D11DeviceContext1)), "ppDeferredContext")]),
    Method(HRESULT, "CreateBlendState1", [(Pointer(Const(D3D11_BLEND_DESC1)), "pBlendStateDesc"), Out(Pointer(ObjPointer(ID3D11BlendState1)), "ppBlendState")]),
    Method(HRESULT, "CreateRasterizerState1", [(Pointer(Const(D3D11_RASTERIZER_DESC1)), "pRasterizerDesc"), Out(Pointer(ObjPointer(ID3D11RasterizerState1)), "ppRasterizerState")]),
    Method(HRESULT, "CreateDeviceContextState", [(D3D11_1_CREATE_DEVICE_CONTEXT_STATE_FLAG, "Flags"), (Array(Const(D3D_FEATURE_LEVEL), "FeatureLevels"), "pFeatureLevels"), (UINT, "FeatureLevels"), (UINT, "SDKVersion"), (REFIID, "EmulatedInterface"), Out(Pointer(D3D_FEATURE_LEVEL), "pChosenFeatureLevel"), Out(Pointer(ObjPointer(ID3DDeviceContextState)), "ppContextState")]),
    Method(HRESULT, "OpenSharedResource1", [(HANDLE, "hResource"), (REFIID, "returnedInterface"), Out(Pointer(ObjPointer(Void)), "ppResource")]),
    Method(HRESULT, "OpenSharedResourceByName", [(LPCWSTR, "lpName"), (DXGI_SHARED_RESOURCE_FLAG, "dwDesiredAccess"), (REFIID, "returnedInterface"), Out(Pointer(ObjPointer(Void)), "ppResource")]),
]

ID3DUserDefinedAnnotation.methods += [
    Method(INT, "BeginEvent", [(LPCWSTR, "Name")]),
    Method(INT, "EndEvent", []),
    Method(Void, "SetMarker", [(LPCWSTR, "Name")]),
    Method(BOOL, "GetStatus", []),
]

d3d11.addInterfaces([
    ID3D11Device1,
    ID3DUserDefinedAnnotation,
])
