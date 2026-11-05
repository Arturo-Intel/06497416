#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;
static const char* vertex = R"GLSL(#version 330 core
void main() {
    const vec2 positions[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
    gl_Position = vec4(positions[gl_VertexID], 0, 1);
}
)GLSL";
std::string read(const fs::path& p){
 std::ifstream f(p,std::ios::binary); if(!f)throw std::runtime_error("Cannot read "+p.string());
 return {std::istreambuf_iterator<char>(f),{}};
}
void write(const fs::path& p,const std::string& s){
 std::ofstream f(p,std::ios::binary); if(!f)throw std::runtime_error("Cannot write "+p.string()); f<<s;
}
void checkGL(const char* where){
 GLenum e=glGetError(); if(e!=GL_NO_ERROR){std::ostringstream s;s<<where<<": GL error 0x"<<std::hex<<e;throw std::runtime_error(s.str());}
}
GLuint shader(GLenum type,const std::string& src,std::string& logs){
 GLuint id=glCreateShader(type);const char* p=src.c_str();glShaderSource(id,1,&p,nullptr);glCompileShader(id);
 GLint ok=0,n=0;glGetShaderiv(id,GL_COMPILE_STATUS,&ok);glGetShaderiv(id,GL_INFO_LOG_LENGTH,&n);
 if(n>1){std::vector<char>b(n);glGetShaderInfoLog(id,n,nullptr,b.data());logs+=b.data();logs+='\n';}
 if(!ok){glDeleteShader(id);throw std::runtime_error("Shader compilation failed\n"+logs);}return id;
}
GLuint program(const std::string& frag,std::string& logs){
 GLuint vs=0,ps=0,p=0;
 try{
 vs=shader(GL_VERTEX_SHADER,vertex,logs);ps=shader(GL_FRAGMENT_SHADER,frag,logs);
 p=glCreateProgram();glAttachShader(p,vs);glAttachShader(p,ps);glLinkProgram(p);
 GLint ok=0,n=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);glGetProgramiv(p,GL_INFO_LOG_LENGTH,&n);
 if(n>1){std::vector<char>b(n);glGetProgramInfoLog(p,n,nullptr,b.data());logs+=b.data();logs+='\n';}
 if(!ok)throw std::runtime_error("Program link failed\n"+logs);
 glDeleteShader(vs);glDeleteShader(ps);return p;
 }catch(...){if(vs)glDeleteShader(vs);if(ps)glDeleteShader(ps);if(p)glDeleteProgram(p);throw;}
}
std::string variant(std::string s,const std::string& weight,const std::string& mode){
 const std::string name="vec3 torchBlockSpecialLightColor";
 size_t begin=s.find(name);if(begin==std::string::npos||s.find(name,begin+name.size())!=std::string::npos)
 throw std::runtime_error("Expected exactly one torch initializer");
 size_t end=s.find(';',begin),pos=s.find("0.75f",begin);
 if(end==std::string::npos||pos==std::string::npos||pos>end)throw std::runtime_error("Unexpected torch initializer");
 const std::map<std::string,std::string> weights={{"literal","0.75f"},{"inline","clamp(0.75f, 0.0f, 1.0f)"},{"helper","clamp01(0.75f)"},{"uniform","reproWeight"}};
 if(weight!="literal")s.replace(pos,5,weights.at(weight));
 if(weight=="uniform"){auto nl=s.find('\n');if(nl==std::string::npos)throw std::runtime_error("No version line");s.insert(nl+1,"uniform float reproWeight = 0.75f;\n");}
 if(mode!="full"){
 auto m=s.find("void main()");if(m==std::string::npos)throw std::runtime_error("Original main not found");
 s.replace(m,11,"void original_main()");
 std::string expr=mode=="initializer"?"torchBlockSpecialLightColor":"GetSpecialBlocklightColor(heldItemId - 44000).rgb";
 s+="\nvoid main(){iris_FragData0=vec4("+expr+",1.0);iris_FragData1=vec4(0.0);}\n";
 }return s;
}
using Values=std::map<std::string,std::vector<float>>;
void uniforms(GLuint p,int size,int held,int frame,float angle,float seconds,int material,const fs::path& log){
 Values values={
 {"iris_FogStart",{0}},{"iris_FogEnd",{256}},{"near",{0.1f}},{"far",{256}},
 {"viewWidth",{float(size)}},{"viewHeight",{float(size)}},{"viewSize",{float(size),float(size)}},
 {"texelSize",{1.f/size,1.f/size}},{"aspectRatio",{1}},{"screenBrightness",{0.5f}},
 {"sunAngle",{angle}},{"eyeAltitude",{64}},{"fogColor",{0.2f,0.3f,0.4f}},
 {"skyColor",{0.3f,0.4f,0.6f}},{"iris_FogColor",{0.2f,0.3f,0.4f,1}},
 {"heldItemId",{float(held)}},{"heldItemId2",{44003}},{"heldBlockLightValue",{15}},
 {"heldBlockLightValue2",{0}},{"frameCounter",{float(frame)}},{"frameTimeCounter",{seconds}},
 {"frameTime",{1.f/60}},{"reproWeight",{0.75f}},{"reproMaterial",{float(material)}},
 {"reproLightmap",{0.6f,0.5f}},{"inVanillaEnd",{1}},{"inNetherWastes",{1}},
 {"isSneaking",{1}},{"isShockwave",{1}},{"atlasSize",{2,2}}};
 GLint count=0,maxlen=0;glGetProgramiv(p,GL_ACTIVE_UNIFORMS,&count);glGetProgramiv(p,GL_ACTIVE_UNIFORM_MAX_LENGTH,&maxlen);
 std::ostringstream f; (void)log; f<<"name,type,value\n";std::vector<char> buf(std::max(maxlen,1));
 for(GLint i=0;i<count;++i){GLsizei len=0;GLint array=0;GLenum type=0;
 glGetActiveUniform(p,i,GLsizei(buf.size()),&len,&array,&type,buf.data());std::string name(buf.data(),len);
 GLint loc=glGetUniformLocation(p,name.c_str());if(loc<0)continue;
 if(array!=1)throw std::runtime_error("Unsupported uniform array: "+name);
 int n=1;bool integer=false;std::vector<float> v;
 switch(type){
 case GL_SAMPLER_2D:v={0};integer=true;break;
 case GL_SAMPLER_2D_SHADOW:v={1};integer=true;break;
 case GL_SAMPLER_3D:v={2};integer=true;break;
 case GL_UNSIGNED_INT_SAMPLER_3D:v={3};integer=true;break;
 case GL_FLOAT_MAT3:case GL_FLOAT_MAT4:{
 int side=type==GL_FLOAT_MAT3?3:4;v.assign(side*side,0);for(int k=0;k<side;++k)v[k*side+k]=1;
 if(side==3)glUniformMatrix3fv(loc,1,GL_FALSE,v.data());else glUniformMatrix4fv(loc,1,GL_FALSE,v.data());break;}
 case GL_FLOAT:break;
 case GL_FLOAT_VEC2:n=2;break;case GL_FLOAT_VEC3:n=3;break;case GL_FLOAT_VEC4:n=4;break;
 case GL_INT:case GL_BOOL:integer=true;break;
 case GL_INT_VEC2:n=2;integer=true;break;case GL_INT_VEC3:n=3;integer=true;break;
 default:throw std::runtime_error("Unsupported uniform: "+name);
 }
 bool matrix=type==GL_FLOAT_MAT3||type==GL_FLOAT_MAT4;
 if(!matrix){
 if(v.empty()){auto it=values.find(name);v=it==values.end()?std::vector<float>(n,0):it->second;}
 if(int(v.size())!=n)throw std::runtime_error("Uniform dimension mismatch: "+name);
 if(integer){std::array<GLint,4>a{};for(int j=0;j<n;++j)a[j]=GLint(v[j]);
 switch(n){case 1:glUniform1iv(loc,1,a.data());break;case 2:glUniform2iv(loc,1,a.data());break;case 3:glUniform3iv(loc,1,a.data());break;case 4:glUniform4iv(loc,1,a.data());break;}}
 else switch(n){case 1:glUniform1fv(loc,1,v.data());break;case 2:glUniform2fv(loc,1,v.data());break;case 3:glUniform3fv(loc,1,v.data());break;case 4:glUniform4fv(loc,1,v.data());break;}
 }
 f<<name<<','<<type<<",\"";for(float x:v)f<<std::setprecision(9)<<x<<' ';f<<"\"\n";
 }checkGL("uniform setup");
}
GLuint inputTexture(int unit,GLenum target,GLint internal,GLenum format,GLenum type,const void* data,bool compare=false){
 GLuint t;glGenTextures(1,&t);glActiveTexture(GL_TEXTURE0+unit);glBindTexture(target,t);
 if(target==GL_TEXTURE_3D)glTexImage3D(target,0,internal,2,2,2,0,format,type,data);
 else glTexImage2D(target,0,internal,2,2,0,format,type,data);
 glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glTexParameteri(target,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(target,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 if(target==GL_TEXTURE_3D)glTexParameteri(target,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
 if(compare){glTexParameteri(target,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);glTexParameteri(target,GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);}
 return t;
}
int execute(int argc,char**argv){
 fs::path sourcePath="minimal330.frag",output="results";int size=128,frame=0,material=10001;float angle=.25f,seconds=1;bool visible=false;std::string only="full";
 for(int i=1;i<argc;++i){std::string a=argv[i];auto next=[&](){if(++i>=argc)throw std::runtime_error("Missing value for "+a);return std::string(argv[i]);};
 if(a=="--shader")sourcePath=next();else if(a=="--out")output=next();else if(a=="--size")size=std::stoi(next());
 else if(a=="--mode")only=next();else if(a=="--frame")frame=std::stoi(next());else if(a=="--sun-angle")angle=std::stof(next());
 else if(a=="--time")seconds=std::stof(next());else if(a=="--material")material=std::stoi(next());else if(a=="--visible")visible=true;
 else if(a=="--help"){std::cout<<"mix_repro [--shader minimal330.frag] [--out results] [--size 128] [--mode full] [--frame 0] [--sun-angle .25] [--time 1] [--material 10001] [--visible]\n";return 0;}
 else throw std::runtime_error("Unknown option: "+a);}
 if(size<1||size>4096)throw std::runtime_error("Size must be 1..4096");
 if(only!="full")throw std::runtime_error("This minimal-color test supports --mode full only");
 std::string source=read(sourcePath);fs::create_directories(output);
 fs::remove(output/"compile_errors.txt");
 glfwSetErrorCallback([](int code,const char* text){std::cerr<<"GLFW "<<code<<": "<<text<<'\n';});
 if(!glfwInit())throw std::runtime_error("glfwInit failed");
 struct Cleanup{GLFWwindow* w=nullptr;~Cleanup(){if(w)glfwDestroyWindow(w);glfwTerminate();}}cleanup;
 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,6);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
 glfwWindowHint(GLFW_VISIBLE,visible?GLFW_TRUE:GLFW_FALSE);
 cleanup.w=glfwCreateWindow(size,size,"Minecraft mix reproducer",nullptr,nullptr);
 if(!cleanup.w)throw std::runtime_error("OpenGL 3.3 context creation failed");glfwMakeContextCurrent(cleanup.w);
 glewExperimental=GL_TRUE;GLenum ge=glewInit();if(ge!=GLEW_OK)throw std::runtime_error(reinterpret_cast<const char*>(glewGetErrorString(ge)));
 while(glGetError()!=GL_NO_ERROR){} // GLEW may leave an error during core-context initialization.
 std::ofstream meta(output/"report.txt");
 if(!meta)throw std::runtime_error("Cannot create report.txt");
 for(auto entry:std::vector<std::pair<std::string,GLenum>>{{"Vendor",GL_VENDOR},{"Renderer",GL_RENDERER},{"OpenGL",GL_VERSION},{"GLSL",GL_SHADING_LANGUAGE_VERSION}}){
 const auto* p=glGetString(entry.second);std::string value=p?reinterpret_cast<const char*>(p):"unknown";meta<<entry.first<<": "<<value<<'\n';std::cout<<entry.first<<": "<<value<<'\n';}
 meta<<"Synthetic inputs, not Minecraft capture replay\nSize: "<<size<<" x "<<size<<"\nFrame: "<<frame<<"\nSun angle: "<<angle<<"\nTime: "<<seconds<<"\nMaterial: "<<material<<'\n';
 write(output/"input_shader.frag",source);write(output/"vertex.vert",vertex);
 GLuint vao;glGenVertexArrays(1,&vao);glBindVertexArray(vao);
 std::vector<float> color(16,.5f),depth(4,1),volume(32,.04f);std::vector<uint32_t> ids(8,2);
 std::array<GLuint,4> inputs={inputTexture(0,GL_TEXTURE_2D,GL_RGBA32F,GL_RGBA,GL_FLOAT,color.data()),
 inputTexture(1,GL_TEXTURE_2D,GL_DEPTH_COMPONENT32F,GL_DEPTH_COMPONENT,GL_FLOAT,depth.data(),true),
 inputTexture(2,GL_TEXTURE_3D,GL_RGBA32F,GL_RGBA,GL_FLOAT,volume.data()),
 inputTexture(3,GL_TEXTURE_3D,GL_R32UI,GL_RED_INTEGER,GL_UNSIGNED_INT,ids.data())};
 glActiveTexture(GL_TEXTURE4);GLuint fbo;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
 std::array<GLuint,2> targets;glGenTextures(2,targets.data());
 for(int i=0;i<2;++i){glBindTexture(GL_TEXTURE_2D,targets[i]);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,size,size,0,GL_RGBA,GL_FLOAT,nullptr);
 glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,GL_TEXTURE_2D,targets[i],0);}
 const GLenum buffers[]={GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};glDrawBuffers(2,buffers);
 if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Float framebuffer incomplete");
 glViewport(0,0,size,size);for(GLenum state:{GL_BLEND,GL_DEPTH_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,GL_DITHER,GL_FRAMEBUFFER_SRGB})glDisable(state);
 glPixelStorei(GL_PACK_ALIGNMENT,1);checkGL("resource setup");
 // This executable validates ONLY fragment shaders that output the expected constant color.
 std::ofstream csv(output/"results.csv");
 if(!csv)throw std::runtime_error("Cannot create results.csv");
 csv<<"variant,status,expected_r,expected_g,expected_b,actual_r0,actual_g0,actual_b0,actual_a0,failed_pixels,nan_components,inf_components,max_abs_finite_error\n";
 const std::array<double,4> expected={2.21,1.0725,6.9075,1.0};
 meta<<"\nTest: direct constant-color output, all pixels and all RGBA channels\n"
     <<"Expected RGBA: (2.21, 1.0725, 6.9075, 1)\n"
     <<"Tolerance: abs_error <= 1e-5 + 1e-5 * abs(expected)\n\n";
 std::cout<<"Expected RGBA: (2.21, 1.0725, 6.9075, 1)\n";
 int compileFailures=0,failedVariants=0;
 std::ofstream errors;
 for(const std::string weight:{"literal","inline","helper","uniform"}){
     std::string frag=variant(source,weight,"full"),logs;
     write(output/(weight+".frag"),frag);
     GLuint p=0;
     try { p=program(frag,logs); }
     catch(const std::exception& e){
         if(!errors.is_open())errors.open(output/"compile_errors.txt",std::ios::trunc);
         errors<<weight<<":\n"<<e.what()<<"\n\n";
         csv<<weight<<",COMPILE_ERROR,,,,,,,,,,,\n";
         meta<<weight<<": COMPILE_ERROR (see compile_errors.txt)\n";
         std::cerr<<weight<<": COMPILE_ERROR\n";
         ++compileFailures;continue;
     }
     if(!logs.empty())meta<<weight<<" compiler/linker messages:\n"<<logs<<'\n';
     glUseProgram(p);
     // Retain the previous input setup, but do not generate uniform dump files.
     uniforms(p,size,44002,frame,angle,seconds,material,{});
     const GLfloat clear[]={-999,-999,-999,-999};
     glClearBufferfv(GL_COLOR,0,clear);glClearBufferfv(GL_COLOR,1,clear);
     glDrawArrays(GL_TRIANGLES,0,3);glFinish();checkGL("draw");
     std::vector<float> pixels(size_t(size)*size*4);
     glReadBuffer(GL_COLOR_ATTACHMENT0);
     glReadPixels(0,0,size,size,GL_RGBA,GL_FLOAT,pixels.data());checkGL("readback");
     size_t failedPixels=0,nans=0,infs=0;double maxError=0;
     for(size_t i=0;i<pixels.size();i+=4){
         bool bad=false;
         for(size_t c=0;c<4;++c){
             double x=pixels[i+c];
             if(std::isnan(x)){++nans;bad=true;}
             else if(std::isinf(x)){++infs;bad=true;}
             else {
                 double error=std::abs(x-expected[c]);maxError=std::max(maxError,error);
                 if(error>1e-5+1e-5*std::abs(expected[c]))bad=true;
             }
         }
         if(bad)++failedPixels;
     }
     std::string status=failedPixels?"FAIL":"PASS";
     if(failedPixels)++failedVariants;
     csv<<std::setprecision(10)<<weight<<','<<status<<",2.21,1.0725,6.9075";
     for(size_t c=0;c<4;++c)csv<<','<<pixels[c];
     csv<<','<<failedPixels<<','<<nans<<','<<infs<<','<<maxError<<'\n';
     std::ostringstream line;
     line<<std::setprecision(10)<<weight<<": "<<status
         <<" | first RGBA=("<<pixels[0]<<", "<<pixels[1]<<", "<<pixels[2]<<", "<<pixels[3]<<")"
         <<" | failed pixels="<<failedPixels<<'/'<<size_t(size)*size
         <<" | NaN="<<nans<<" Inf="<<infs<<" | max finite error="<<maxError;
     meta<<line.str()<<'\n';std::cout<<line.str()<<'\n';
     glDeleteProgram(p);
 }
 meta<<"\nSummary: "<<failedVariants<<" numerical failures, "<<compileFailures<<" compilation failures\n";
 meta<<"Exit codes: 0=all pass, 1=fatal error, 2=compilation failure, 3=numerical failure\n";
 glDeleteTextures(4,inputs.data());glDeleteTextures(2,targets.data());glDeleteFramebuffers(1,&fbo);glDeleteVertexArrays(1,&vao);
 std::cout<<"Results: "<<output<<'\n';return compileFailures?2:(failedVariants?3:0);
}
int main(int argc,char** argv){try{return execute(argc,argv);}catch(const std::exception& e){std::cerr<<"Fatal: "<<e.what()<<'\n';return 1;}}
