#ifndef ECON_ITEM_SYSTEM_H
#define ECON_ITEM_SYSTEM_H

#ifdef _WIN32
#pragma once
#endif


class CEconSchemaParser;
class CEconItemDefinition;
struct EconAttributeDefinition;
struct EconQuality;
struct EconColor;

enum
{
	ATTRTYPE_INVALID = -1,
	ATTRTYPE_INT,
	ATTRTYPE_UINT64,
	ATTRTYPE_FLOAT,
	ATTRTYPE_STRING
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconItemSchema
{
	friend class CEconSchemaParser;
	friend class CTFInventory;
public:
	CEconItemSchema();
	~CEconItemSchema();

	bool Init( void );
	void Precache( void );

	CEconItemDefinition* GetItemDefinition( int id );
	EconAttributeDefinition *GetAttributeDefinition( int id );
	EconAttributeDefinition *GetAttributeDefinitionByName( const char* name );
	EconAttributeDefinition *GetAttributeDefinitionByClass( const char* name );
	int GetAttributeIndex( const char *classname );
	int GetAttributeType( const char *type ) const;

	KeyValues *m_pSchema;

protected:
	CUtlDict< int, unsigned short >					m_GameInfo;
	CUtlDict< EconQuality, unsigned short >			m_Qualities;
	CUtlDict< EconColor, unsigned short >			m_Colors;
	CUtlDict< KeyValues *, unsigned short >			m_PrefabsValues;
	CUtlMap< int, CEconItemDefinition * >			m_Items;
	CUtlMap< int, EconAttributeDefinition * >		m_Attributes;

private:
	bool m_bInited;
};

CEconItemSchema *GetItemSchema();

#endif // ECON_ITEM_SYSTEM_H
