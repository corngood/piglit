// [config]
// expect_result: pass
// glsl_version: 1.40
// require_extensions: GL_ARB_enhanced_layouts GL_ARB_shader_storage_buffer_object
// check_link: false
// [end config]
//
// ARB_enhanced_layouts spec says:
//    "The *offset* qualifier can only be used on block members of blocks
//    declared with *std140* or *std430* layouts.
//
// Tests for successful compilation, when the block is of std140 layout.
//

#version 140
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shader_storage_buffer_object : enable

layout(std140) buffer b {
       layout(offset = 0) vec4 var1;
       layout(offset = 32) vec4 var2;
};

void main()
{
}
