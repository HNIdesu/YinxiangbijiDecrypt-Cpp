#include <filesystem>
#include <vector>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include "utility.hpp"
#include "logger.hpp"
#include "openssl/hmac.h"
#include "openssl/evp.h"
#include "xercesc/dom/DOM.hpp"
#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/parsers/XercesDOMParser.hpp"
#include "xercesc/framework/LocalFileFormatTarget.hpp"

using namespace std;
using namespace xercesc;
namespace fs=std::filesystem;
const char TAG[]="yinxiangbijidecrypt";
const char HMACKEY[]="{22C58AC3-F1C7-4D96-8B88-5E4BBF505817}";

static void print_help_text(){
    Logger::log(TAG,"","\n",{"Usage: yinxiangbijidecrypt <file_path|directory_path>"});
    return;
}

static unique_ptr<Byte> base64_decode(char* input,uint32_t& result_length){
    uint32_t readPtr=0,writePtr=0,count=0;
    for (auto ch=input[readPtr];ch;ch=input[++readPtr]){
        if(ch!='\r'&&ch!='\n'&&ch!=' ')
            input[writePtr++]=ch;
        if(ch=='=')count++;
    }
    input[writePtr]='\0';
    auto pbuffer=unique_ptr<Byte>(new Byte[writePtr/4*3]);
    result_length= EVP_DecodeBlock(pbuffer.get(),(const Byte*)input,writePtr)-count;
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

class XmlStringGC{
private:
vector<XMLCh*>* xmlStrList;
vector<char*>* cStrList;
public:
    char* add(char* cStr){
        cStrList->push_back(cStr);
        return cStr;
    }
    XMLCh* add(XMLCh* xmlStr){
        xmlStrList->push_back(xmlStr);
        return xmlStr;
    }
    XmlStringGC(){
        xmlStrList=new vector<XMLCh*>();
        cStrList=new vector<char*>();
    }
    ~XmlStringGC(){
        for(auto xmlStr:*xmlStrList)
            XMLString::release(&xmlStr);
        for(auto cStr:*cStrList)
            XMLString::release(&cStr);
        delete xmlStrList;
        delete cStrList;
    }
};

static void save_xml_to_file(const char* filepath,DOMDocument* dom,XmlStringGC* gc){
    auto const impl=DOMImplementationRegistry::getDOMImplementation(gc->add(XMLString::transcode("LS")));
    auto const serializer= impl->createLSSerializer();
    auto const output=impl->createLSOutput();
    LocalFileFormatTarget target(filepath);
    output->setByteStream(&target);
    serializer->write(dom,output);
    serializer->release();
    output->release();
}

static void decrypt_file(const char* filepath){
    Logger::log(TAG," ","\n",{"Starting to dectypt",filepath});
    xercesc::XercesDOMParser parser;
    XmlStringGC gc;
    parser.setLoadExternalDTD(false); 
    parser.setValidationScheme(XercesDOMParser::Val_Never); 
    parser.parse(filepath);
    auto const dom=parser.getDocument();
    if (!dom) {
        Logger::error(TAG," ","\n",{"Failed to parse document:",filepath});
        return;
    }
    auto const root=dom->getDocumentElement();
    if (!root) {
        Logger::error(TAG," ","\n",{"Root element is null in file:",filepath});
        return;
    }
    auto const theNotes=root->getElementsByTagName(gc.add(XMLString::transcode("note")));
    uint32_t totalCount=0,decryptedCount=0;
    for(XMLSize_t i=0,noteCount=theNotes->getLength();i<noteCount;i++){
        totalCount++;
        XmlStringGC gc1;
        try{
            auto const theNote=(DOMElement*)theNotes->item(i);
            const char* const title=gc1.add(XMLString::transcode(theNote->getElementsByTagName(gc1.add(XMLString::transcode("title")))->item(0)->getTextContent()));
            Logger::log(TAG," ","\n",{"Decrypting note:",title});
            auto const contentElement=(DOMElement*)theNote->getElementsByTagName(gc1.add(XMLString::transcode("content")))->item(0);
            const char* const encoding=gc1.add(XMLString::transcode(contentElement->getAttribute(gc1.add(XMLString::transcode("encoding")))));
            if(!strcmp(encoding,"base64:aes")){
                auto child=contentElement->getFirstChild();
                auto const base64Content=gc1.add(XMLString::transcode(contentElement->getTextContent()));
                uint32_t encrypted_data_length;
                auto content_data = base64_decode(base64Content,encrypted_data_length);
                auto const note_length=decrypt_note(content_data.get(),encrypted_data_length);
                auto const raw_data=content_data.get();
                contentElement->replaceChild(dom->createCDATASection(gc1.add(XMLString::transcode((const char*)raw_data))),child);
                contentElement->removeAttribute(gc1.add(XMLString::transcode("encoding")));
            }else
                continue;
            decryptedCount++;
        }catch(exception ex){
            Logger::error("Decryption failed","","\n",{},&ex);
        }
        
    }
    Logger::log(TAG," ","\n",{"Decryption succeeded,",to_string(decryptedCount).c_str(),"notes decrypted,",to_string(totalCount).c_str(),"notes in total"});
    fs::path newpath(filepath);
    newpath.replace_extension(".enex");
    save_xml_to_file(newpath.c_str(),dom,&gc);
    Logger::log(TAG," ","\n",{"File has been saved to",newpath.c_str()});
}

int main(int argc,char** argv){
    if(argc<=1){
        print_help_text();
        return 0;
    }
    try{
        XMLPlatformUtils::Initialize();
        const char* const path=argv[1];
        if(!fs::exists(path))
        {
            Logger::error(TAG," ","\n",{"Path",path,"not exists"});
            return 0;
        }
        if(fs::is_directory(path)){
            for(auto const entry : fs::directory_iterator(path))
                if(!strcmp(entry.path().extension().c_str(),".notes"))
                    decrypt_file(entry.path().c_str());
        }else
            decrypt_file(path);
        Logger::log(TAG,"","\n",{"File decryption completed"});
        XMLPlatformUtils::Terminate();
    }catch(exception ex){
        Logger::error(TAG," ","\n",{"A unknown exception occurred"},&ex);
    }
    return 0;
}