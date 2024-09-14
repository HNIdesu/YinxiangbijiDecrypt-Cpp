#ifdef _WIN32
#include <windows.h>
#endif
#include <filesystem>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include "utility.hpp"
#include "Path.hpp"
#include "logger.hpp"
#include "openssl/hmac.h"
#include "openssl/evp.h"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/framework/LocalFileFormatTarget.hpp"
#include <xercesc/util/TransService.hpp>
#include "xercesc/util/PlatformUtils.hpp"

using namespace std;
using namespace xercesc;
namespace fs=std::filesystem;
const char TAG[]="yinxiangbijidecrypt";
const char HMACKEY[]="{22C58AC3-F1C7-4D96-8B88-5E4BBF505817}";

static void print_help_text(){
    Logger::log(TAG,"","\n",{"Usage: yinxiangbijidecrypt <file_path|directory_path>"});
    return;
}
bool flag=false;
static unique_ptr<Byte[]> base64_decode(const char* input,uint32_t& result_length){
    auto const ctx=EVP_ENCODE_CTX_new();
    EVP_DecodeInit(ctx);
    auto const raw_length=strlen(input);
    auto pbuffer=unique_ptr<Byte[]>(new Byte[raw_length/4*3]);
    uint32_t decoded_length=0,i=0;
    int out_length;
    for (auto ch=input[i];ch;ch=input[i]){
        if(ch=='\r'||ch=='\n')
        {
            i++;
            continue;
        }
        EVP_DecodeUpdate(ctx,pbuffer.get()+decoded_length,&out_length,(const Byte*)input+i,4);
        decoded_length+=out_length;
        i+=4;
    }
    EVP_DecodeFinal(ctx,pbuffer.get()+decoded_length,&out_length);
    EVP_ENCODE_CTX_free(ctx);
    decoded_length+=out_length;
    result_length= decoded_length;
    return pbuffer;
}

static void hmac_sha256(
    const Byte* key,
    uint32_t key_length,
    const Byte* data,
    uint32_t data_length,
    Byte* result,
    uint32_t* output_length){
    Byte buffer[32];
    HMAC(EVP_sha256(),
         key, key_length,
         data,
         data_length, 
         buffer, 
         output_length); 
    memcpy(result,buffer,32);
}

static void generate_key(const Byte* nonce,Byte* output){
    Byte buffer[32];
    uint32_t input_length=20;
    memcpy(buffer,nonce,16);
    *((int*)(buffer+16))=0x01000000;
    memset(output,0,16);
    for(int i=0;i<50000;i++){
        hmac_sha256((const Byte*)HMACKEY,sizeof(HMACKEY)-1,buffer,input_length,buffer,&input_length);
        for(int j=0;j<16;j++)
            output[j]^=buffer[j];
    }
}

static void aes_cbc_decrypt(
    const Byte* key,
    const Byte* iv,
    const Byte* input,
    uint32_t input_length,
    Byte* result,
    uint32_t& result_length){
    auto const ctx=EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx,EVP_aes_128_cbc(),nullptr,key,iv);
    int length1=0,length2=0;
    EVP_DecryptUpdate(ctx,result,&length1,input,input_length);
    EVP_DecryptFinal(ctx,result+length1,&length2);
    result_length=length1+length2;
    EVP_CIPHER_CTX_free(ctx);
}

static uint32_t decrypt_note(Byte* encrypted_data,int encrypted_data_length){
    Byte key1[16];
    Byte key2[16];
    Byte hash[32];
    uint32_t bytes_read;
    memory_stream ms(encrypted_data,encrypted_data_length,true);
    if(memcmp(ms.read(4,bytes_read),"ENC0",4))
        throw runtime_error("Signature verify failed");
    generate_key(ms.read(16,bytes_read),key1);
    generate_key(ms.read(16,bytes_read),key2);
    auto const iv=ms.read(16,bytes_read);
    auto const encrypted_note_length=encrypted_data_length - 4 - 16 * 5;
    auto const encrypted_content=ms.read(encrypted_note_length,bytes_read);
    hmac_sha256(key2,16,encrypted_data,encrypted_data_length-32,hash,&bytes_read);
    if(memcmp(hash,ms.read(32,bytes_read),32))
        throw runtime_error("Hash verify failed");
    aes_cbc_decrypt(key1,iv,encrypted_content,encrypted_note_length,encrypted_data,bytes_read);
    return bytes_read-1;
}

class XmlStringTranscoder{
private:
    vector<u16string*>* xmlStrList;
    vector<string*>* cStrList;
    wstring_convert<codecvt_utf8_utf16<XMLCh>,XMLCh> convert;
public:
    const XMLCh* transcode(const char* cStr){
        auto const result=new u16string(convert.from_bytes(cStr));
        xmlStrList->push_back(result);
        return result->c_str();
    }
    const char* transcode(const XMLCh* xmlStr){
        auto const result=new string(convert.to_bytes(xmlStr));
        cStrList->push_back(result);
        return result->c_str();
    }
    XmlStringTranscoder(){
        xmlStrList=new vector<u16string*>();
        cStrList=new vector<string*>();
    }
    ~XmlStringTranscoder(){
        for(auto const s : *xmlStrList)
            delete s;
        for(auto const s : *cStrList)
            delete s;
        delete xmlStrList;
        delete cStrList;
    }
};

static void save_xml_to_file(const char* filepath,xercesc::DOMDocument* dom,XmlStringTranscoder* gc){
    auto const impl=DOMImplementationRegistry::getDOMImplementation(gc->transcode("LS"));
    auto const serializer= impl->createLSSerializer();
    auto const output=impl->createLSOutput();
    LocalFileFormatTarget target(gc->transcode(filepath));
    output->setByteStream(&target);
    serializer->write(dom,output);
    serializer->release();
    output->release();
}

static void decrypt_file(string filepath){
    Logger::log(TAG," ","\n",{"Starting to dectypt",filepath.c_str()});
    xercesc::XercesDOMParser parser;
    XmlStringTranscoder tr1;
    parser.setLoadExternalDTD(false);
    parser.setValidationScheme(XercesDOMParser::Val_Never);
    parser.parse(tr1.transcode(filepath.c_str()));
    auto const dom=parser.getDocument();
    if (!dom) {
        Logger::error(TAG," ","\n",{"Failed to parse document:",filepath.c_str()});
        return;
    }
    auto const root=dom->getDocumentElement();
    if (!root) {
        Logger::error(TAG," ","\n",{"Root element is null in file:",filepath.c_str()});
        return;
    }
    auto const theNotes=root->getElementsByTagName(tr1.transcode("note"));
    uint32_t totalCount=0,decryptedCount=0;
    for(XMLSize_t i=0,noteCount=theNotes->getLength();i<noteCount;i++){
        totalCount++;
        XmlStringTranscoder tr2;
        try{
            auto const theNote=(DOMElement*)theNotes->item(i);
            const char* const title=tr2.transcode(theNote->getElementsByTagName(tr2.transcode("title"))->item(0)->getTextContent());
            Logger::log(TAG," ","\n",{"Decrypting note:",title});
            auto const contentElement=(DOMElement*)theNote->getElementsByTagName(tr2.transcode("content"))->item(0);
            const char* const encoding=tr2.transcode(contentElement->getAttribute(tr2.transcode("encoding")));
            if(!strcmp(encoding,"base64:aes")){
                auto child=contentElement->getFirstChild();
                auto const base64Content=tr2.transcode(contentElement->getTextContent());
                uint32_t encrypted_data_length;
                auto content_data = base64_decode(base64Content,encrypted_data_length);
                auto const note_length=decrypt_note(content_data.get(),encrypted_data_length);
                auto const raw_data=content_data.get();
                contentElement->replaceChild(dom->createCDATASection(tr2.transcode((const char*)raw_data)),child);
                contentElement->removeAttribute(tr2.transcode("encoding"));
            }else
                continue;
            decryptedCount++;
        }catch(exception ex){
            Logger::error("Decryption failed","","\n",{},&ex);
        }
        
    }
    Logger::log(TAG," ","\n",{"Decryption succeeded,",to_string(decryptedCount).c_str(),"notes decrypted,",to_string(totalCount).c_str(),"notes in total"});
    auto const newpath=Path(filepath).ReplaceExtension(".enex").ToUtf8String();
    save_xml_to_file(newpath.c_str(),dom,&tr1);
    Logger::log(TAG," ","\n",{"File has been saved to",newpath.c_str()});
}

int main(int argc,char** argv){
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    if(argc<=1){
        print_help_text();
        return 0;
    }
    unique_ptr<Path> path;
#ifdef _WIN32
    auto const cp=GetACP();
    auto path_length=strlen(argv[1]);
    auto path_buffer=new wchar_t[path_length+1];
    path_buffer[MultiByteToWideChar(cp,NULL,argv[1],path_length,path_buffer,path_length+1)]=L'\0';
    path=unique_ptr<Path>(new Path(path_buffer));
#else
    path=unique_ptr<Path>(new Path(argv[1]));
#endif
    if(!path->Exists())
    {
        auto const u8path=path->ToUtf8String();
        Logger::error(TAG," ","\n",{"Path",u8path.c_str(),"not exists"});
        return 0;
    }
    try{
        XMLPlatformUtils::Initialize();
        if(path->IsDirectory()){
            for(auto const entry : path->EnumerateFiles()){
                Path entryPath(entry.path());
                if(entryPath.HasExtension(".notes"))
                    decrypt_file(entryPath.ToUtf8String());
            }
        }else
            decrypt_file(path->ToUtf8String());
        Logger::log(TAG,"","\n",{"File decryption completed"});
        XMLPlatformUtils::Terminate();
    }catch(exception ex){
        Logger::error(TAG," ","\n",{"A unknown exception occurred"},&ex);
    }
    return 0;
}