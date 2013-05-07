#ifndef __SCRIPTOR_H__
#define __SCRIPTOR_H__


/** 
xml-->script
*/

#include "cocos2d.h"
#include "utils/FileIO.h"
#include "tinyxml/tinyxml.h"

USING_NS_CC;
using namespace std;

typedef enum
{
	sUnknown = 0,
	sFilePack,
	sReadFile,
	sScriptPack,
	sAct,
	sShowText,
	sSelect,
	sChoice,
	sLoadImg,
	sAction,
	sFinal,
	sLock,
	sSilent,
	sList,
	sItem,
	sMapscp,
	sNameNode,
	sTypeNode,
	sChange,
	sMapAction
}ScriptType;

typedef enum
{
	vInt = 0,
	vFloat
}ValueType;

class NFloat : public CCObject
{
public:
	float f;

	NFloat(float fv){
		f = fv;
	}
};



class Script : public CCObject{
public:
	ScriptType type;	
	string content;

	int m_anum, m_snum;

	CCDictionary* attributes;
	CCArray* scriptnodes; 
	CCDictionary* mapscpnodes;

	virtual void release(){
		//CCObject::release();
		if(m_uReference < 3)
			delete this;
		else
			CCObject::release();
	}

	virtual CCObject* autorelease(){
		return CCObject::autorelease();
	}

	Script(){
		type = sUnknown;
		m_snum = 0;
		m_anum = 0;
		attributes = NULL;
		scriptnodes = NULL;
		mapscpnodes = NULL;
		//autorelease();
	}

	~Script(){
		//if(attributes) {
		//	//CCDictElement* tcde;
		//	//CCDICT_FOREACH(attributes,tcde){
		//	//	if(tcde->getObject()->retainCount()>1)
		//	//		tcde->getObject()->release();
		//	//}
		//	//attributes->removeAllObjects();
		//	attributes->release();
		//}


		CC_SAFE_RELEASE_NULL(attributes);

		//if(scriptnodes) scriptnodes->removeAllObjects();
		CC_SAFE_RELEASE_NULL(scriptnodes);

		//if(mapscpnodes) mapscpnodes->removeAllObjects();
		CC_SAFE_RELEASE_NULL(mapscpnodes);
	}

	const char* getstring(const char* tag){

		CCString* t_cs = (CCString*) attributes->objectForKey(tag);
		if(!t_cs) return "";
		
		return t_cs->getCString();
	}

	int getint(const char* tag){
		if(!attributes) return 0;
		CCInteger* t_cs = (CCInteger*) attributes->objectForKey(tag);
		if(!t_cs) return 0;
		return t_cs->getValue();
	}

	float getfloat(const char* tag){
		NFloat* nf = (NFloat*) attributes->objectForKey(tag);
		if(!nf) return 0;
		return nf->f;
	}

	
	
};



const unsigned int NUM_INDENTS_PER_SPACE=2;

class Scriptor {

private:
	FileIO* m_curfi;
	Script* m_scWhole;
	

public:
	CCArray* initcs;
	CCArray* m_caScript;
	CCDictionary* mapscps;
	int sn,in;

	Scriptor(){
		m_scWhole = NULL;
		initcs = NULL;
		m_caScript = NULL;
		mapscps = NULL;
	}

	void re_init(){
		if(mapscps) mapscps->removeAllObjects();
		if(m_caScript) m_caScript->removeAllObjects();
		if(initcs)	initcs->removeAllObjects();

		CC_SAFE_RELEASE_NULL(initcs);
		CC_SAFE_RELEASE_NULL(m_caScript);
		CC_SAFE_RELEASE_NULL(mapscps);
	}

	bool parse_string(string line){
		do 
		{
			CC_BREAK_IF(line.size()<1);
			re_init();
			initcs = new CCArray();
			mapscps = new CCDictionary();			
			
			TiXmlDocument doc;
			doc.Parse(line.c_str());


			sn = 0;
			in = 0;
			dump_to_nodes(&doc);

			return true;
		} while (0);
		return false;
	}

	// isRaw and password is for the case that the xml is packed in zip.
	bool parse_file(const char* fname, const char* password = "1",bool isRaw = false){
		//FileIO* fi = new FileIO(fname,password);		//not used,just test under
		//////////////////////////////////////////////////////////////////////////
		do 
		{
			CCLOG(">REad File Prepare...");
			unsigned long filesize;
			string path = CCFileUtils::sharedFileUtils()->fullPathFromRelativePath(fname);			//未打包
			char *buffer = (char *)CCFileUtils::sharedFileUtils()->getFileData(path.c_str(), "rb", &filesize);

			CC_BREAK_IF(!buffer);
			CCLOG(">Read over and scc");

			re_init();

			TiXmlDocument doc;
			doc.Parse(buffer);
			initcs = new CCArray();
//			mapscps = new CCDictionary();
			
			sn = 0;
			in = 0;
			dump_to_nodes(&doc);
			
			return true;
		} while (0);
		return false;
	}

	CCDictionary* dump_attribs_to_nodes(TiXmlElement* pElement, unsigned int indent, ScriptType st = sUnknown)
	{
		if ( !pElement ) return NULL;
		if ( st == sUnknown) return NULL;		//关闭调试输出

		CCDictionary* dt = new CCDictionary(); 

		TiXmlAttribute* pAttrib=pElement->FirstAttribute();
		int i=0;
		int ival;
		double dval;

		while (pAttrib)
		{
			//CCLOG( "%s%s: value=[%s]", pIndent, pAttrib->Name(), pAttrib->Value());
			if (pAttrib->QueryIntValue(&ival)==TIXML_SUCCESS)   
			{
				CCInteger* ci = new CCInteger(ival);
				dt->setObject(ci,pAttrib->Name());
				ci->release();
			}
			else if (pAttrib->QueryDoubleValue(&dval)==TIXML_SUCCESS) 
			{
				NFloat* fl = new NFloat(dval);
				dt->setObject(fl,pAttrib->Name());
				fl->release();
			}
			else 
			{
				CCString* sl = new CCString(pAttrib->Value());
				dt->setObject(sl,pAttrib->Name());
				pAttrib->Value();
				CCLOG("value pair:%s||%s||%s",pAttrib->Value(),pAttrib->Name(),sl->getCString());
				sl->release();
			}
			i++;
			pAttrib=pAttrib->Next();
		}
		if(i>0)	return dt;
		else CC_SAFE_RELEASE_NULL(dt);
			
		return NULL;
	}

	Script* dump_to_nodes( TiXmlNode* pParent, unsigned int indent = 0, ScriptType st = sUnknown)
	{
		if ( !pParent ) return NULL;

		ScriptType tp = st;
		TiXmlNode* pChild;
		TiXmlText* pText;
		int t = pParent->Type();

		CCDictionary* ct = NULL;
		CCArray* ca = NULL;
		CCDictionary* cd = NULL;

		switch ( t )
		{
		case TiXmlNode::DOCUMENT:
			CCLOG( "-Document" );
			break;

		case TiXmlNode::ELEMENT:
			{
				const char* el_name = pParent->Value();
				CCLOG( "Element [%s]", el_name );
			
				if(strcmp(el_name,"pack") == 0) tp = sFilePack;
				else if(strcmp(el_name,"lock") == 0) tp = sLock;
				else if(strcmp(el_name,"list") == 0) tp = sList;
				else if(strcmp(el_name,"png") == 0) tp = sReadFile;
				else if(strcmp(el_name,"scripts") == 0) tp = sScriptPack;
				else if(strcmp(el_name,"text") == 0) tp = sShowText;
				else if(strcmp(el_name,"act") == 0) tp = sAct;
				else if(strcmp(el_name,"select") == 0) tp = sSelect;
				else if(strcmp(el_name,"choice") == 0) tp= sChoice;
				else if(strcmp(el_name,"loadimg") == 0) tp = sLoadImg;
				else if(strcmp(el_name,"action") == 0) tp = sAction;
				else if(strcmp(el_name,"final") == 0) tp = sFinal;		
				else if(strcmp(el_name,"silent") == 0) tp = sSilent;
				else if(strcmp(el_name,"item") == 0) tp = sItem;
				else if(strcmp(el_name,"mapscp") == 0) tp = sMapscp;
				else if(strcmp(el_name,"change") == 0) tp = sChange;
				else if(strcmp(el_name,"mapaction") == 0) tp = sMapAction;


				ct = dump_attribs_to_nodes(pParent->ToElement(), indent+1,tp);	//num of attrib
			
				break;
			}
		case TiXmlNode::COMMENT:
			CCLOG( "Comment: [%s]", pParent->Value());
			break;

		case TiXmlNode::UNKNOWN:
			CCLOG( "Unknown" );
			break;

		case TiXmlNode::TEXT:
			pText = pParent->ToText();
			CCLOG( "Text: [%s],we do not use text.", pText->Value() );
			break;

		case TiXmlNode::DECLARATION:
			CCLOG( "-Declaration" );
			break;
		default:
			break;
		}

		bool t_bNeedRet = true;
		int m = 0;
		CCLOG("----One sccin.Indent:%d----",indent);
		if(ct != NULL){													
			CCInteger* cod = (CCInteger*) ct->objectForKey("total");	//a_这里只_读取script,_mapscp不_提供total属性。
			if(NULL != cod)
			{
				int ccon = cod->getValue();								
				CCLOG("---Indent:%d||Total:%d---",indent,ccon);

				ca = new CCArray(ccon);
				 pChild = pParent->FirstChild();
				for(;m<ccon;m++)
				{
					ca->insertObject(dump_to_nodes( pChild, indent+1, tp),m);
					pChild = pChild->NextSibling();
				}

			}
		}
		int k = 0;
		if(m<1)
		{
			
			ca = new CCArray();
			cd = new CCDictionary();
			for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
			{
				if(tp == sMapscp){
					Script* mtm = dump_to_nodes(pChild, indent+1, sNameNode);
					cd->setObject(mtm,mtm->getstring("name"));
					CCLOG("---Indent:%d||NT,sMapscp:%s---",indent,mtm->getstring("name"));
					k++;
				}else if(tp == sNameNode){
					Script* mtm = dump_to_nodes(pChild, indent+1, sTypeNode);
					cd->setObject(mtm,mtm->getint("type"));
					CCLOG("---Indent:%d||NT,sNameNode:%d---",indent,mtm->getint("type"));
					k++;
				}else if(tp != sUnknown)											//_以上为msp的读入
				{
					ca->insertObject(dump_to_nodes( pChild, indent+1, tp),k);
					CCLOG("---Indent:%d||NT,sUnknown:%d---",indent,k);
					k++;
				}
				else{
					CCLOG("-Per sable Leak,sUnknown Node:%d",indent);
					dump_to_nodes( pChild, indent+1, tp);
					
				}
				
				
			}	

		}


		Script* tmps = new Script();
		if(k+m > 0) {
			if(!ct) ct = new CCDictionary();
			if(m==0){
				CCInteger* t_i = new CCInteger(k);
				ct->setObject(t_i,"total");
				t_i->release();
			}


			tmps->scriptnodes = ca;
			tmps->mapscpnodes = cd;
		}else{							
			CCLOG("-Leaf Node Detect");

			CC_SAFE_RELEASE_NULL(ca);
			CC_SAFE_RELEASE_NULL(cd);
		}

		tmps->m_snum = k + m;
		tmps->attributes = ct;	
		tmps->type = tp;

		switch(tp){
		case sFilePack:
			{
				initcs->addObject(tmps);
				in++;
				return NULL;
			}
		case sUnknown:
			{
				CC_SAFE_RELEASE_NULL(tmps);
				return NULL;
			}
		case sScriptPack:
			{
				m_caScript = ca;
				//if(ca) ca->retain();
				sn = k + m;
				tmps->scriptnodes = NULL;
			}
		case sList:
			{
				initcs->addObject(tmps);
				in++;
				return NULL;
			}
		case sMapscp:
			{
				mapscps = cd;
				//if(cd) cd->retain();
				tmps->mapscpnodes = NULL;
				CC_SAFE_RELEASE_NULL(tmps);
				return NULL;
			}
		default:
			{
				return tmps;

			}
		}


	}

	~Scriptor(){
		CC_SAFE_DELETE(m_scWhole);

		//if(mapscps) mapscps->removeAllObjects();
		CC_SAFE_RELEASE_NULL(mapscps);

		//if(m_caScript) m_caScript->removeAllObjects();
		CC_SAFE_RELEASE_NULL(m_caScript);

		//if(initcs)	initcs->removeAllObjects();
		CC_SAFE_RELEASE_NULL(initcs);
		
		
		
	}

};

#endif

