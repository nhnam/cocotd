#include "System.h"

#include "pthread.h"

#include "GameScene.h"

#include "GameManager.h"
#include "SoundManager.h"
#include "CCSpriterX.h"

#include "utils/Scriptor.h"
#include "utils/ScirptDer.h"
#include "sublayer/InfoTab.h"
#include "utils/EffectControler.h"
#include "packui/InterfaceEffect.h"


#define SELTAG 1010


using namespace cocos2d;

typedef enum
{
	tLySpalash = 0,
	tLyMap,
	tLySpot,
	tLyCG,
	tLyText,
	tLyMovie,
	tLyMod
}LayerTag;

CCDictionary* Imgstack = NULL;


// �߼�����ת�Ƶ�����
bool GameScene::init()
{
    bool bRet = false;
    do 
    {
        CC_BREAK_IF(! CCScene::init());
		ml = NULL;
		snapshot = NULL;

		m_StageState = 0;
		CC_SAFE_RELEASE_NULL(Imgstack);
		this->scheduleUpdate();
		
        bRet = true;

    } while (0);

    return bRet;
}
//////////////////////////////////////////////////////////////////////////
// pThread for init.

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool initover;
int sum, coun;
Scriptor* sp;
string fscp;

GameScene::~GameScene(){
	;
}

void cleanup(void *arg){  
	initover = true;
}  

void *initscript(void *arg){ 
	//pthread_mutex_lock(&mutex);
	CCLOG(">T_Scrip Begin");
	sp = new Scriptor();
	sp->parse_file(fscp.c_str());
	CCLOG(">T_Scrip End");
	pthread_mutex_unlock(&mutex);
	return ((void *)0); 
}

void *initmusic(void *arg){ 
	CCLOG(">T_Music:Begin.");
	//SoundManager::sharedSoundManager()->PreLoadSrc("d");	
	CCArray* mss = sp->initcs;
	int i = 0;
	Script* s;
	for(;i<mss->count();++i){
		s = (Script*) mss->objectAtIndex(i);
		if(s->getint("type") == 2) break;			// Initcs -- Type == 2 -- Music Initilize. Ψһ
		CC_SAFE_RELEASE_NULL(s);
	}
	CCLOG(">T_Music:Preparing Buffer.");
	CCArray* sss = s->scriptnodes;
	Script* t;
	for(i=0;i<s->m_snum;++i){
		t = (Script*) sss->objectAtIndex(i);
		SoundManager::sharedSoundManager()->PreLoadSrc(t->getstring("path"));
	}
	CCLOG(">T_Music:Return.");
	return ((void *)0);
}

void initImg(Script* sp){
	CCLOG(">InitImg. Thread. Begin.%s",sp->getstring("path"));
	CCImage* image = new CCImage();
	unsigned long filesize;
	FileIO* fi = new FileIO(sp->getstring("path"));

	for(int i = 0;i<sp->m_snum;++i){
		Script* t = (Script*) sp->scriptnodes->objectAtIndex(i);	//TODO:���ݺ�׺���ı�Image�Ķ�������
		CCLOG(">>Inert Img. %s. Begin.", t->getstring("path"));

		unsigned char* pBuffer = fi->getFileData(t->getstring("copname"),&filesize);
		image->initWithImageData((void*)pBuffer, filesize, CCImage::kFmtPng);
		ImageInfo *pImageInfo = new ImageInfo();
		pImageInfo->image = image;
		pImageInfo->imageType = CCImage::kFmtPng;		

		Imgstack->setObject(pImageInfo,t->getstring("path"));	//TODO:���ݽű�������Դ

		CCLOG(">>Inert Img. %s. End.", t->getstring("path"));
	}



	CCLOG(">InitImg. Thread. Out.");
	CC_SAFE_DELETE(fi);
}

void *initfiles(void *arg){ 
	CCLOG(">T_InitFile:Begin.");
	static int counter = 0;

//	pthread_cleanup_push(cleanup,"thread 1 first handler");
	

	int err;
	pthread_t tid2;
		//,music;		
	pthread_mutex_lock(&mutex);
	err = pthread_create(&tid2,NULL,initscript,(void *)1); 
	if( err != 0){  
		fprintf(stderr,"create thread2 failed: %s",strerror(err));  
		exit(1);  
	} 
	//pthread_join(tid2,NULL);
	CCLOG(">PT_LOCK");
	pthread_mutex_lock(&mutex);
	CCLOG(">PT_LOCK_PASS");
	//Img Cache Begin.Use Png only.
	CCArray* mss = sp->initcs;
	int i = 0;
	Script* s;
	for(;i<mss->count();++i){
		s = (Script*) mss->objectAtIndex(i);
		if(s->getint("type") == 1) initImg(s);			// Initcs -- Type == 1 -- Png Pack. Multi.
		CC_SAFE_RELEASE_NULL(s);
	}


//	pthread_create(&music,NULL,initmusic,(void *)1);
//	pthread_join(music,NULL);
	pthread_mutex_unlock(&mutex);

	CCLOG(">T_InitFile:Return.");
	initover = true;
//	pthread_cleanup_pop(1);  
	return ((void *)0);   
}
//////////////////////////////////////////////////////////////////////////
bool GameScene::f_cachetest(const char* rscMask){	//�ƶ����沢��¼��map
	do 
	{	
		ImageInfo *img = (ImageInfo *) m_pImages->objectForKey(rscMask);
		CC_BREAK_IF(!img);

		CCTextureCache::sharedTextureCache()->addUIImage(img->image,rscMask);
		img->release();
		m_pImages->removeObjectForKey(rscMask);
	} while (0);
	return true;
}

void GameScene::f_initover(){
	this->removeChildByTag(tLySpalash,true);	//�Ƴ�������splash��

	m_pImages = Imgstack;
	ScriptList = sp->scripts;
	m_IScriptSum = sp->sn;
	
	//////////////////////////////////////////////////////////////////////////
	
	

	ml = new MapLayer();
	this->addChild(ml,tLyMap);
	
	ml->f_init();
	AddState(ml);


	//�����￪ʼչ��textlayer
	ImageInfo *img = (ImageInfo *) m_pImages->objectForKey("Images/background.png");
	CCTextureCache::sharedTextureCache()->addUIImage(img->image,"Images/background.png");
	img->release();

	//������
	bg = new CCLayer();
	BgImg = NULL;
	this->addChild(bg,tLyCG);
	

	m_IsTextShown = false;
	te = new TextLayer();
	e_curscript = 0;
	addChild(te,tLyText);
	AddState(te);
	
	m_IsTextShown = true;
	e_TextAuto = false;
	//
	tMovie = new MovieLayer();
	AddState(tMovie);
	this->addChild(tMovie,tLyMovie);

	//tMod = new ModelLayer();
	//AddState(tMod);
	//addChild(tMod,tLyMod);

	//textlayer�������������			���Ͼ��ǽ���textlayer��Ҫ�����

	StateCenter* t_sc = StateCenter::sharedStateCenter();
	if(t_sc->m_bIsLoad){
		if(! t_sc->g_load_file()) exit(0x5001);
		e_curscript = t_sc->m_iJump;
		//e_State = t_sc->m_oldstate;
		t_curscript = t_sc->m_iTJump;
		te->ELoad();

		m_sBgi = t_sc->m_sBgi;
		addBackImg();
		tMovie->cleanupplayer();
		e_update(t_sc->m_oldstate);

		if(e_State == 1){
			tScriptList = ml->tm->f_scrfind(ml->tm->m_sNaScp,ml->tm->m_iNaScp)->scriptnodes;
			t_ssum = tScriptList->count();
		}

		e_act();
		if(e_State != t_sc->m_iState) {
			CCLOG("Loading may be incorrect!Tring to fix it automaticly.");
			e_update(t_sc->m_iState);
		}
		m_bLoadProtect = true;

	}else{
		SoundManager::sharedSoundManager()->PlayMusic("sound/1.ogg");

		e_update(0);
		e_act();
		m_bLoadProtect = false;
	}


}

void GameScene::TextChange(float dt){
	//if(!m_IsTextShown){
	//	m_IsTextShown = true;

	//}else{

	//	
	//	m_IsTextShown = false;
	//}
	//te->setVisible(m_IsTextShown);
}

bool GameScene::e_getscript(){
	return false;
}

void GameScene::onExit(){
	CCScene::onExit();
}

void GameScene::update(float dt)	//��������scene�ĳ�ʼ��
{
	switch(m_StageState){
	case(0):			// 0 --> stageδ��ʼ��
		{
			visibleSize = CCDirector::sharedDirector()->getVisibleSize();
			origin = CCDirector::sharedDirector()->getVisibleOrigin();
			initover = false;
			Imgstack = new CCDictionary();
			m_pImages = NULL;

			StateCenter* t_sc = StateCenter::sharedStateCenter();
			if(t_sc->m_bIsLoad){
				scpfile = t_sc->m_sName;
			}


			fscp = scpfile;
			
	
			//GameManager::sharedLogicCenter()->getPosition();
			int err;  
			pthread_t tid1;	
			///thread moving to end?

			err = pthread_create(&tid1,NULL,initfiles,(void *)1);  

			if( err != 0){  
				fprintf(stderr,"create thread1 failed: %s",strerror(err));  
				exit(1);  
			} 



			//////////////////////////////////////////////////////////////////////////

			CCLayer* SplashLayer = new CCLayer();
			CCLabelTTF* pLabel = CCLabelTTF::create("LOADING...", "Arial", 24);
			CC_BREAK_IF(! pLabel);

			// Get window size and place the label upper. 
			CCSize size = CCDirector::sharedDirector()->getWinSize();
			pLabel->setPosition(ccp(size.width / 2, size.height - 50));

			// Add the label to HelloWorld layer as a child layer.
			SplashLayer->addChild(pLabel, 1);
			SplashLayer->setTouchEnabled(false);

			CCSpriteFrameCache *cache = CCSpriteFrameCache::sharedSpriteFrameCache();
			CCSpriteBatchNode *sheet = CCSpriteBatchNode::create("sprite/monster.png");
			cache->addSpriteFramesWithFile("sprite/monster.plist");
			SplashLayer->addChild(sheet);
			CCSpriterX *animator;
			animator = CCSpriterX::create("sprite/Example.SCML");


			animator->setPosition(ccp(400, 100));
			animator->setAnchorPoint(ccp(0,0));
			animator->setScale(0.8f);
			animator->PlayAnim("Idle");

			sheet->addChild(animator);

			this->addChild(SplashLayer,100,tLySpalash);
			//SoundManager::sharedSoundManager()->PlayMusic();
			//SoundManager::sharedSoundManager()->PreLoadSrc("d");
			//////////////////////////////////////////////////////////////////////////

			m_StageState = 1;
			break;
		}
	case(1):			// 1 --> stage��ʼ����
		{
			if(initover) m_StageState = 2;
			else CCLOG("Init being called.Render the splash.");
			break;
		}
	case(2):			//2 --> stage��ʼ�����
		{
			f_initover();	
			m_StageState = 3;
			unscheduleUpdate();
			break;
		}

	case(3):			//3 --> stage����ѭ��
		{
			if(m_bCanSnap && m_bScrLock){
				m_bScrLock = false;
				unscheduleUpdate();
				e_act();
			}
			break;
			//unschedule(update);
		}
	default:
		{
			//CCLOG("!!!!!!!!!!!Unkonwn stage state.");
			exit(1);
		}
	}
} 

void GameScene::Snap(){
	if(!m_bCanSnap) return;
	if(snapshot) CC_SAFE_RELEASE_NULL(snapshot);
	CCDirector::sharedDirector()->setNextDeltaTimeZero(true);
	CCSize winsize = CCDirector::sharedDirector()->getWinSize();
	//CCLayerColor* whitePage = CCLayerColor::layerWithColor(ccc4(255, 255, 255, 0), winsize.width, winsize.height);
	CCRenderTexture* rtx = CCRenderTexture::renderTextureWithWidthAndHeight(winsize.width, winsize.height);
	rtx->setPosition(winsize.width / 2, winsize.height / 2);
	//rtx->beginWithClear(1,1,1,0);
	rtx->begin();
	CCDirector::sharedDirector()->getRunningScene()->visit();
	rtx->end();
	//snapshot = new CCImage();
	snapshot = rtx->newCCImage();
}

void GameScene::PrepareSave(){
	Snap();
	StateCenter::sharedStateCenter()->f_get_state();

}

void GameScene::PreQuit(){
	PrepareSave();
	StatesManager::PreQuit();
	ml->Close();
}

void GameScene::e_update(int new_State)	
{
	if(e_State == new_State) return;
	las_state = e_State;
	switch(e_State){
	case 3:
		{
			tMovie->Close();
			break;
		}
	case 1:
		{
			m_bLoadProtect = false;
			break;
		}
	}



	switch(new_State){
	case 0:		//showing texts
		{
			ActiveOnly(te);
			break;
		}
	case 1:		//showing msp texts
		{
			ActiveOnly(te);			
			break;
		}
	case 2:		//��ͼ����
		{
			m_bLoadProtect = false;
			OpenOnly(ml);
			break;
		}
	case 3:		//MoviePlayer;
		{
			ActiveOnly(tMovie);
			te->Close();
			break;
		}
	default:	//unknown state
		{
		}
	}
	e_State = new_State;
}

void GameScene::e_tmpscript(CCArray* sl,int sum){
	if(e_State == 1 && m_bLoadProtect){
		return;
	}
	tScriptList = sl;
	t_ssum = sum;
	t_curscript = 0;
	e_update(1);
	e_act();
}

void GameScene::e_handlecurs(Script* s){
	int t_sum = ((CCInteger*) s->attributes->objectForKey("total"))->getValue();
	CCArray* acts = s->scriptnodes;
	//CCLOG("script handle.");
	for (int i = 0;i<t_sum;i++)		//multi here?
	{
		//CCLOG("handle scripte:%d",i);
		Script* tmp = (Script*) acts->objectAtIndex(i);//use tag to define node's having state
		switch(tmp->type)
		{
		case sShowText:
			{
				te->ShowText(tmp->getstring("content"));
				break;
			}
		case sSelect:
			{
				te->DerSelMenu(tmp);
				break;
			}
		case sLoadImg:
			{
				te->DerLoadImg(tmp);
				break;
			}
		case sAction:
			{
				te->DerAction(tmp);
				break;
			}
		case sFinal:
			{
				te->DerFinal(tmp);
				break;
			}
		case sLock:
			{
				te->DerLock(tmp);
				break;
			}
		case sSilent:
			{
				te->DerSilent(tmp);
				break;
			}
		case sChange:
			{
				DerChange(tmp);
				break;
			}
		case sMapAction:
			{
				ml->tm->DerMapAct(tmp);
				break;
			}
		default:
			break;
		}
	}
}

void GameScene::addBackImg(){
	if(m_sBgi.length()<1) return;
	f_cachetest(m_sBgi.c_str());
	BgImg = CCSprite::create(m_sBgi.c_str());
	BgImg->setPosition(ccp(visibleSize.width/2 + origin.x, visibleSize.height/2 + origin.y));
	bg->addChild(BgImg);
}

void GameScene::DerChange(Script* s){
	int type = s->getint("type");
	switch(type){
	case 0:	//��������
		{
			if(BgImg) 
			{
				BgImg->removeFromParent();
				//CC_SAFE_RELEASE(BgImg);
				BgImg = NULL;
			}
			m_sBgi = s->getstring("content");
			addBackImg();
			
			break;
		}
	case 1:  //�������
		{
			if(BgImg) 
			{
				BgImg->removeFromParent();
				m_sBgi.clear();
				BgImg = NULL;
			}
			break;
		}
	case 2:	//��̬���뷽��������ml����һ���յ�tm����ֹ�Ƿ�����
		{
			ml->openTileMap(s->getstring("content"),s->getstring("script"));
			break;
		}
	case -2:
		{
			ml->openBattleMap(s->getstring("content"),0);
			break;
		}
	case 3:	//��ͼ�����뷽��>>1.��ʼ��ʱͳһ���벢������ml��.2.����ʱ����ʹ��case2.���ǵ���ͬ��ͼʹ�ò�ͬ�ű����������һ�ֿ�����Ʒ��룬�ڶ���ʵʱ�ԱȽϺá�
		{
			if(BgImg){
				BgImg->removeFromParent();
				BgImg = NULL;
				
			}
			bg->setVisible(false);	
			ml->f_resumeall();			
			ml->f_resumecon();
			e_update(2);
			break;
		}
	case 4:	//ֹͣ��ͼһ�л����ΪĿǰ��û�йرյ�ͼ���ƵĲ��ԣ���tilemap�м�����ر��������ƣ�Ŀǰ��Ҫ��ml�Ĺ��ܽ���ǿ��
		{
			ml->tm->pauseSchedulerAndActions();
			break;
		}
	case 5:	//������Ƶ
		{
			tMovie->set_media(s->getstring("content"));
			e_update(3);
			break;
		}
	case 6://test 
		{
			InfoTab* it = InfoTab::sharedInfoTab();
			it->showinfo(s->getstring("content"));
			
			break;
		}
	case 7:
		{
			//static bool test_lock = true;					//��ֹ���ܲ�������Խű��ĳ�ͻ��test only��
			//if(test_lock){
				EffectControler::sharedEffectControler()->md_use_item(NULL,s->getint("id"));
/*				test_lock = false;
			}
	*/		
			break;
		}
	}
}

void GameScene::e_act(){
	if(!m_bCanSnap){
		m_bScrLock = true;
		scheduleUpdate();
		return;
	}
	las_state = e_State;
	switch(e_State){
	case 0:	//Txt�ű�
		{
			if(e_curscript<m_IScriptSum){
				//CCLOG("cur 0 script:%d",e_curscript);
				Script* cur = (Script*) ScriptList->objectAtIndex(e_curscript);
				e_handlecurs(cur);
			}
			break;
		}
	case 1://��ͼ�ű�
		{
			//ml->tm->pauseSchedulerAndActions();				//TODO:���ýű����ƶ�����ʹ��ͳһ����Ϊ
			//ml->f_stopcontrol();
			if(t_curscript<t_ssum){
				Script* cur = (Script*) tScriptList->objectAtIndex(t_curscript);
				e_handlecurs(cur);
			}else{
				e_update(2);
			}
			break;
		}

	}

}

void GameScene::e_gonext(){
	//CCLOG("Go next being called!!!!!!");
	switch(e_State){
	case 0:
		{
			e_curscript++;
			e_act();
			break;
		}
	case 1:
		{
			t_curscript++;
			e_act();
			break;
		}

	}

}

void GameScene::e_jump(int j){
	//CCLOG("e_jump being called!!!");
	switch(e_State){
	case 0:
		{
			if(j<0) {
				e_curscript++;
			}else{
				e_curscript = j;			
			}
			e_act();
			break;
		}
	case 1:
		{
			if(j<0) {
				t_curscript++;
			}else{
				t_curscript = j;			
			}
			e_act();
			break;
		}

	}



	//CCLOG("Jump!:%d",j);
}