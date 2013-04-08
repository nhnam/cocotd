#ifndef __ITEM_CELL_H__
#define __ITEM_CELL_H__

/*
*	������ʾ��Ʒ����Ŀ�Ļ���Ԫ�ء�
*
*/
#include "packui/Container.h"

struct ItemCellData : public CCObject			//Struct for ItemCellData,ֻ�����Ҫ�������Թ���ʾ�����е����ݹ�����listdbsource�ṩ
{
	int type_id;
	int sum;
	int lock;

	string name;
	string icon_mask;

	ItemCellData(int ti, int s, int l){
		type_id = ti;
		sum		= s;
		lock	= l;
	}

};

class ItemCell : public Container
{
protected:
	int m_iType;

public:
	int sum,lock;

	ItemCell(float w, float h, int type, ItemCellData* a_icd);

	virtual void onNormal();
	virtual void onHover();
	virtual bool ccTouchBegan(CCTouch *pTouch, CCEvent *pEvent){
		return false;
	};

	void f_init(float w, float h, int type, ItemCellData* a_icd);
	void f_setsum(int i);

	void setNormal();
	void setValue();
};


#endif