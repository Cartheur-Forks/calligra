/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kexiquerydesignerguieditor.h"

#include <qlayout.h>
#include <qpainter.h>
#include <qdom.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kexidb/field.h>
#include <kexidb/queryschema.h>
#include <kexidb/connection.h>
#include <kexidb/parser/parser.h>
#include <kexidb/parser/sqlparser.h>

#include <kexiproject.h>
#include <keximainwindow.h>
#include <kexiinternalpart.h>
#include <kexitableview.h>
#include <kexitableitem.h>
#include <kexitableviewdata.h>
#include <kexidragobjects.h>
#include "kexiquerypart.h"
#include "kexidialogbase.h"
#include "kexidatatable.h"
#include "kexi_utils.h"
#include "kexisectionheader.h"
#include "kexitableviewpropertybuffer.h"
#include "widget/relations/kexirelationwidget.h"
#include "widget/relations/kexirelationviewtable.h"

class KexiQueryDesignerGuiEditorPrivate
{
public:
	KexiQueryDesignerGuiEditorPrivate()
	 : droppedNewItem(0)
	 , slotTableAdded_enabled(true)
	{
	}

	KexiTableViewData *data;
	KexiDataTable *dataTable;
	QGuardedPtr<KexiDB::Connection> conn;

	KexiRelationWidget *relations;
	KexiSectionHeader *head;
	QSplitter *spl;

	//! used to remember in slotDroppedAtRow() what data was dropped, 
	//! so we can create appropriate prop. buffer in slotRowInserted()
	KexiTableViewData *fieldColumnData, *tablesColumnData;
	KexiTableViewPropertyBuffer* buffers;
	KexiTableItem *droppedNewItem;

	QString droppedNewTable, droppedNewField;

	bool slotTableAdded_enabled : 1;
};

//=========================================================

KexiQueryDesignerGuiEditor::KexiQueryDesignerGuiEditor(
	KexiMainWindow *mainWin, QWidget *parent, const char *name)
 : KexiViewBase(mainWin, parent, name)
 , d( new KexiQueryDesignerGuiEditorPrivate() )
{
	d->conn = mainWin->project()->dbConnection();
//	m_doc = doc;

	d->spl = new QSplitter(Vertical, this);
	d->spl->setChildrenCollapsible(false);
//	KexiInternalPart::createWidgetInstance("relation", win, s, "relation");
	d->relations = new KexiRelationWidget(mainWin, d->spl, "relations");
	connect(d->relations, SIGNAL(tableAdded(KexiDB::TableSchema&)),
		this, SLOT(slotTableAdded(KexiDB::TableSchema&)));
	connect(d->relations, SIGNAL(tableHidden(KexiDB::TableSchema&)),
		this, SLOT(slotTableHidden(KexiDB::TableSchema&)));
	connect(d->relations, SIGNAL(tableFieldDoubleClicked(KexiDB::TableSchema*,const QString&)),
		this, SLOT(slotTableFieldDoubleClicked(KexiDB::TableSchema*,const QString&)));

//	addActionProxyChild( m_view->relationView() );
/*	KexiRelationPart *p = win->relationPart();
	if(p)
		p->createWidget(s, win);*/

	d->head = new KexiSectionHeader(i18n("Query columns"), Vertical, d->spl);
//	s->setResizeMode(d->head, QSplitter::KeepSize);
	d->dataTable = new KexiDataTable(mainWin, d->head, "guieditor_dataTable", false);
	d->dataTable->tableView()->setSpreadSheetMode();
//	d->dataTable->tableView()->addDropFilter("kexi/field");
//	setFocusProxy(d->dataTable);

	d->data = new KexiTableViewData(); //just empty data
	d->buffers = new KexiTableViewPropertyBuffer( this, d->dataTable->tableView() );
	initTableColumns();
	initTableRows();
//	d->dataTable->tableView()->setData(d->data);

//	d->buffers = new KexiTableViewPropertyBuffer( this, d->dataTable->tableView() );

//	//last column occupies the rest of the area
//	d->dataTable->tableView()->setColumnStretchEnabled( true, d->data->columnsCount()-1 ); 
//	d->dataTable->tableView()->setColumnStretchEnabled(true, 0);
//	d->dataTable->tableView()->setColumnStretchEnabled(true, 1);
//	d->dataTable->tableView()->adjustHorizontalHeaderSize();
	QValueList<int> c;
	c << 0 << 1 << 4;
	d->dataTable->tableView()->maximizeColumnsWidth( c ); 
//	d->dataTable->tableView()->adjustColumnWidthToContents(-1);
	d->dataTable->tableView()->adjustColumnWidthToContents(2);//'visible'
	d->dataTable->tableView()->setDropsAtRowEnabled(true);
	connect(d->dataTable->tableView(), SIGNAL(dragOverRow(KexiTableItem*,int,QDragMoveEvent*)),
		this, SLOT(slotDragOverTableRow(KexiTableItem*,int,QDragMoveEvent*)));
	connect(d->dataTable->tableView(), SIGNAL(droppedAtRow(KexiTableItem*,int,QDropEvent*,KexiTableItem*&)),
		this, SLOT(slotDroppedAtRow(KexiTableItem*,int,QDropEvent*,KexiTableItem*&)));
	connect(d->data, SIGNAL(aboutToChangeCell(KexiTableItem*,int,QVariant,KexiDB::ResultInfo*)),
		this, SLOT(slotBeforeCellChanged(KexiTableItem*,int,QVariant,KexiDB::ResultInfo*)));
	connect(d->data, SIGNAL(rowInserted(KexiTableItem*,uint,bool)), 
		this, SLOT(slotRowInserted(KexiTableItem*,uint,bool)));
	connect(d->relations, SIGNAL(tablePositionChanged(KexiRelationViewTableContainer*)),
		this, SLOT(slotTablePositionChanged(KexiRelationViewTableContainer*)));
	connect(d->relations, SIGNAL(aboutConnectionRemove(KexiRelationViewConnection*)),
		this, SLOT(slotAboutConnectionRemove(KexiRelationViewConnection*)));

//	m_table = new KexiTableView(d->data, s, "designer");
	QVBoxLayout *l = new QVBoxLayout(this);
	l->addWidget(d->spl);

	addChildView(d->relations);
//	addActionProxyChild(d->relations);
//	setViewWidget(d->relations, false);
	addChildView(d->dataTable);
	setViewWidget(d->dataTable, true);
//	addActionProxyChild(d->dataTable);
//	restore();
	d->relations->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	d->head->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	updateGeometry();
//	d->spl->setResizeMode(d->relations, QSplitter::KeepSize);
//	d->spl->setResizeMode(d->head, QSplitter::FollowSizeHint);
	d->spl->setSizes(QValueList<int>()<< 800<<400);
}

KexiQueryDesignerGuiEditor::~KexiQueryDesignerGuiEditor()
{
}

void
KexiQueryDesignerGuiEditor::initTableColumns()
{
	KexiTableViewColumn *col1 = new KexiTableViewColumn(i18n("Column"), KexiDB::Field::Enum);

	d->fieldColumnData = new KexiTableViewData(KexiDB::Field::Text, KexiDB::Field::Text);
	col1->setRelatedData( d->fieldColumnData );
	d->data->addColumn(col1);

	KexiTableViewColumn *col2 = new KexiTableViewColumn(i18n("Table"), KexiDB::Field::Enum);
	d->tablesColumnData = new KexiTableViewData(KexiDB::Field::Text, KexiDB::Field::Text);
	col2->setRelatedData( d->tablesColumnData );
	d->data->addColumn(col2);

	KexiTableViewColumn *col3 = new KexiTableViewColumn(i18n("Visible"), KexiDB::Field::Boolean);
	d->data->addColumn(col3);

	KexiDB::Field *f = new KexiDB::Field(i18n("Totals"), KexiDB::Field::Enum);
	QValueVector<QString> totalsTypes;
	totalsTypes.append( i18n("Group by") );
	totalsTypes.append( i18n("Sum") );
	totalsTypes.append( i18n("Average") );
	totalsTypes.append( i18n("Min") );
	totalsTypes.append( i18n("Max") );
	//todo: more like this
	f->setEnumHints(totalsTypes);
	KexiTableViewColumn *col4 = new KexiTableViewColumn(*f);
	d->data->addColumn(col4);

/*TODO
f= new KexiDB::Field(i18n("Sort"), KexiDB::Field::Enum);
	QValueVector<QString> sortTypes;
	sortTypes.append( i18n("Ascending") );
	sortTypes.append( i18n("Descending") );
	sortTypes.append( i18n("No sorting") );
	f->setEnumHints(sortTypes);
	KexiTableViewColumn *col5 = new KexiTableViewColumn(*f);
	d->data->addColumn(col5);*/

	KexiTableViewColumn *col6 = new KexiTableViewColumn(i18n("Criteria"), KexiDB::Field::Text);
	d->data->addColumn(col6);

//	KexiTableViewColumn *col7 = new KexiTableViewColumn(i18n("Or"), KexiDB::Field::Text);
//	d->data->addColumn(col7);
}

void KexiQueryDesignerGuiEditor::initTableRows()
{
	d->data->deleteAllRows();
	const int columns = d->data->columnsCount();
	for (int i=0; i<(int)d->buffers->size(); i++) {
		KexiTableItem *item = new KexiTableItem(columns);
		d->data->append(item);
	}
	d->dataTable->tableView()->setData(d->data);

	updateColumnsData();
}

void KexiQueryDesignerGuiEditor::updateColumnsData()
{
	d->dataTable->tableView()->acceptRowEdit();

	QStringList sortedTableNames;
	for (TablesDictIterator it(*d->relations->tables());it.current();++it)
		sortedTableNames += it.current()->table()->name();
	qHeapSort( sortedTableNames );

	//several tables can be hidden now, so remove rows for these tables
	QValueList<int> rowsToDelete;
	for (int r = 0; r<(int)d->buffers->size(); r++) {
		KexiPropertyBuffer *buf = d->buffers->at(r);
		if (buf) {
			QString tableName = (*buf)["table"].value().toString();
			if (sortedTableNames.end() == qFind( sortedTableNames.begin(), 
				sortedTableNames.end(), tableName ))
			{
				//table not found: mark this line for later remove
				rowsToDelete += r;
			}
		}
	}
	d->data->deleteRows( rowsToDelete );

	//update 'table' and 'field' columns
	d->tablesColumnData->deleteAllRows();
	d->fieldColumnData->deleteAllRows();

	KexiTableItem *item = new KexiTableItem(2);
	(*item)[0]="*";
	(*item)[1]="*";
	d->fieldColumnData->append( item );

	for (QStringList::const_iterator it = sortedTableNames.constBegin(); 
		it!=sortedTableNames.constEnd(); ++it)
	{
		//table
		KexiDB::TableSchema *table = d->relations->tables()->find(*it)->table();
		item = new KexiTableItem(2);
		(*item)[0]=table->name();
		(*item)[1]=(*item)[0];
		d->tablesColumnData->append( item );
		//fields
		item = new KexiTableItem(2);
		(*item)[0]=table->name()+".*";
		(*item)[1]=(*item)[0];
		d->fieldColumnData->append( item );
		for (KexiDB::Field::ListIterator t_it = table->fieldsIterator();t_it.current();++t_it) {
			item = new KexiTableItem(2);
			(*item)[0]=table->name()+"."+t_it.current()->name();
			(*item)[1]=QString("  ") + t_it.current()->name();
			d->fieldColumnData->append( item );
		}
	}
//TODO
}

KexiRelationWidget *KexiQueryDesignerGuiEditor::relationView() const
{
	return d->relations;
}

void
KexiQueryDesignerGuiEditor::addRow(const QString &tbl, const QString &field)
{
	kexipluginsdbg << "KexiQueryDesignerGuiEditor::addRow(" << tbl << ", " << field << ")" << endl;
	KexiTableItem *item = new KexiTableItem(0);

//	 = QVariant(tbl);
	item->push_back(QVariant(tbl));
	item->push_back(QVariant(field));
	item->push_back(QVariant(true));
	item->push_back(QVariant());
	d->data->append(item);

	//TODO: this should deffinitly not go here :)
//	m_table->updateContents();

	setDirty(true);
}

KexiQueryPart::TempData *
KexiQueryDesignerGuiEditor::tempData() const
{	
	return static_cast<KexiQueryPart::TempData*>(parentDialog()->tempData());
}

static QString msgCannotSwitch_EmptyDesign() { 
	return i18n("Cannot switch to data view, because query design is empty.\n"
		"First, please create your design.");
}

bool
KexiQueryDesignerGuiEditor::buildSchema(QString *errMsg)
{
	//build query schema
	KexiQueryPart::TempData * temp = tempData();
	if (temp->query)
		temp->query->clear();
	else
		temp->query = new KexiDB::QuerySchema();

	//add tables
	for (TablesDictIterator it(*d->relations->tables()); it.current(); ++it) {
		temp->query->addTable( it.current()->table() );
	}

	//add relations (looking for connections)
	for (ConnectionListIterator it(*d->relations->connections()); it.current(); ++it) {
		KexiRelationViewTableContainer *masterTable = it.current()->masterTable();
		KexiRelationViewTableContainer *detailsTable = it.current()->detailsTable();

		temp->query->addRelationship(
			masterTable->table()->field(it.current()->masterField()),
			detailsTable->table()->field(it.current()->detailsField()) );
	}

	//add fields
	KexiDB::BaseExpr *whereExpr = 0;
	KexiTableViewData::Iterator it(d->data->iterator());
	bool fieldsFound = false;
	for (int i=0; i<(int)d->buffers->size(); ++it, i++) {
		KexiPropertyBuffer *buf = d->buffers->at(i);
		if (buf) {
			QString tableName = (*buf)["table"].value().toString().stripWhiteSpace();
			QString fieldName = (*buf)["field"].value().toString();
			bool fieldVisible = (*buf)["visible"].value().toBool();
			QString criteriaStr = (*buf)["criteria"].value().toString();
			QCString alias = (*buf)["alias"].value().toCString();
			KexiDB::BaseExpr *criteriaExpr = 0;
			if (!criteriaStr.isEmpty()) {
				criteriaExpr = parseCriteriaString(fieldName, criteriaStr);
				if (!criteriaExpr) {//for sanity
					if (errMsg)
						*errMsg = i18n("Invalid criteria \"%1\"").arg(criteriaStr);
					delete whereExpr;
					return false;
				}
				//critera ok: add it to WHERE section
				if (whereExpr)
					whereExpr = new KexiDB::BinaryExpr(KexiDBExpr_Logical, whereExpr, AND, criteriaExpr);
				else //first expr.
					whereExpr =	criteriaExpr;
			}
			if (tableName.isEmpty()) {
				//expresion?
				//TODO
			}
			else if (tableName=="*") {
				//all tables asterisk
				temp->query->addAsterisk( new KexiDB::QueryAsterisk( temp->query, 0 ), fieldVisible );
				if (fieldVisible)
					fieldsFound = true;
				continue;
			}
			else {
				KexiDB::TableSchema *t = d->conn->tableSchema(tableName);
//				if (fieldName.find(".*")!=-1) {
				if (fieldName=="*") {
					//single-table asterisk: <tablename> + ".*" + number
					temp->query->addAsterisk( new KexiDB::QueryAsterisk( temp->query, t ), fieldVisible );
					if (fieldVisible)
						fieldsFound = true;
				} else {
					if (!t) {
						kexipluginswarn << "query designer: NO TABLE '" << (*buf)["table"].value().toString() << "'" << endl;
						continue;
					}
					KexiDB::Field *f = t->field( fieldName );
					if (!f) {
						kexipluginswarn << "query designer: NO FIELD '" << fieldName << "'" << endl;
						continue;
					}
					temp->query->addField(f, fieldVisible);
					if (fieldVisible)
						fieldsFound = true;
					if (!alias.isEmpty())
						temp->query->setColumnAlias( temp->query->fieldCount()-1, alias );
				}
			}
		}
		else {//!buf
			kexipluginsdbg << it.current()->at(1).toString() << endl;
			if (!it.current()->at(1).isNull() && it.current()->at(0).isNull()) {
				kexipluginsdbg << "no field provided!" << endl;
//TODO: show message about missing field name, and set focus to that cell
			}
		}
	}
	if (!fieldsFound) {
		if (errMsg)
			*errMsg = msgCannotSwitch_EmptyDesign();
		return false;
	}
	if (whereExpr)
		kexipluginsdbg << "KexiQueryDesignerGuiEditor::buildSchema(): setting CRITERIA: " << whereExpr->debugString() << endl;

	//set always, because if whereExpr==NULL, 
	//this will clear prev. expr
	temp->query->setWhereExpression( whereExpr );

	//TODO?
	return true;
}

tristate
KexiQueryDesignerGuiEditor::beforeSwitchTo(int mode, bool &dontStore)
{
	kexipluginsdbg << "KexiQueryDesignerGuiEditor::beforeSwitch()" << mode << endl;
	if (mode==Kexi::DesignViewMode) {
		//todo
		return true;
	}
	else if (mode==Kexi::DataViewMode) {
		if (!d->dataTable->tableView()->acceptRowEdit())
			return cancelled;

		if (!dirty() && parentDialog()->neverSaved()) {
			KMessageBox::information(this, msgCannotSwitch_EmptyDesign());
			return cancelled;
		}
		if (dirty() || !tempData()->query) {
			//remember current design in a temporary structure
			dontStore=true;
			QString errMsg;
			//build schema; problems are not allowed
			if (!buildSchema(&errMsg)) {
				KMessageBox::information(this, errMsg);
				return cancelled;
			}
		}
		//TODO
		return true;
	}
	else if (mode==Kexi::TextViewMode) {
		dontStore=true;
		//build schema; ignore problems
		buildSchema();
/*		if (tempData()->query && tempData()->query->fieldCount()==0) {
			//no fields selected: let's add "*" (all-tables asterisk), 
			// otherwise SQL statement will be invalid
			tempData()->query->addAsterisk( new KexiDB::QueryAsterisk( tempData()->query ) );
		}*/
		//todo
		return true;
	}

	return false;
}

tristate
KexiQueryDesignerGuiEditor::afterSwitchFrom(int mode)
{
	if (mode==Kexi::NoViewMode || (mode==Kexi::DataViewMode && !tempData()->query)) {
		//this is not a SWITCH but a fresh opening in this view mode
		if (!m_dialog->neverSaved()) {
			if (!loadLayout()) {
				//err msg
				parentDialog()->setStatus(parentDialog()->mainWin()->project()->dbConnection(), i18n("Query definition loading failed."), i18n("Query data may be corrupted."));
				return false;
			}
			showFieldsForQuery( static_cast<KexiDB::QuerySchema *>(parentDialog()->schemaData()) );
			//todo: load global query properties
		}
	}
	else if (mode==Kexi::TextViewMode) {
		if (tempData()->queryChangedInPreviousView) {
			//previous view changed query data
			//-clear and regenerate GUI items
			d->relations->clear();
			initTableRows();
			//todo
			if (tempData()->query) {
				//there is a query schema to show
				showTablesAndConnectionsForQuery( tempData()->query );
				//-show fields
				showFieldsForQuery( tempData()->query );
			}
		}
		//todo: load global query properties
	}
	else if (mode==Kexi::DataViewMode) {
		//this is just a SWITCH from data view
		//set cursor if needed:
		if (d->dataTable->tableView()->currentRow()<0 
			|| d->dataTable->tableView()->currentColumn()<0)
		{
			d->dataTable->tableView()->ensureCellVisible(0,0);
			d->dataTable->tableView()->setCursor(0,0);
		}
	}
	tempData()->queryChangedInPreviousView = false;
	setFocus(); //to allow shared actions proper update
	return true;
}


KexiDB::SchemaData*
KexiQueryDesignerGuiEditor::storeNewData(const KexiDB::SchemaData& sdata, bool &/*cancel*/)
{
	buildSchema();
	KexiQueryPart::TempData * temp = tempData();
	(KexiDB::SchemaData&)*temp->query = sdata; //copy main attributes

	bool ok = m_mainWin->project()->dbConnection()->storeObjectSchemaData( *temp->query, true /*newObject*/ );
	m_dialog->setId( temp->query->id() );

	if (ok)
		ok = storeLayout();

	KexiDB::QuerySchema *query = temp->query;
	temp->query = 0; //will be returned, so: don't keep it
	if (!ok) {
		delete query;
		return 0;
	}
	return query;
}

tristate KexiQueryDesignerGuiEditor::storeData()
{
	tristate res = KexiViewBase::storeData();
	if (~res)
		return res;
	if (res) {
		buildSchema();
		res = storeLayout();
	}
	return res;
}

void KexiQueryDesignerGuiEditor::showTablesAndConnectionsForQuery(KexiDB::QuerySchema *query)
{
	d->relations->clear();
	d->slotTableAdded_enabled = false; //speedup

	//-show tables:
	//(todo: instead of hiding all tables and showing some tables, 
	// show only these new and hide these unncecessary; the same for connections)
	for (KexiDB::TableSchema::ListIterator it(*query->tables()); it.current(); ++it) {
		d->relations->addTable( it.current() );
	}
	//-show relationships:
	KexiDB::Relationship *rel;
	for (KexiDB::Relationship::ListIterator it(*query->relationships()); (rel=it.current()); ++it) {
		SourceConnection conn;
//@todo: now only sigle-field relationships are implemented!
		KexiDB::Field *masterField = rel->masterIndex()->fields()->first();
		KexiDB::Field *detailsField = rel->detailsIndex()->fields()->first();
		conn.masterTable = masterField->table()->name(); //<<<TODO
		conn.masterField = masterField->name();
		conn.detailsTable = detailsField->table()->name();
		conn.detailsField = detailsField->name();
		d->relations->addConnection( conn );
	}

	d->slotTableAdded_enabled = true;
	updateColumnsData();
}

void KexiQueryDesignerGuiEditor::showFieldsForQuery(KexiDB::QuerySchema *query)
{
	const bool was_dirty = dirty();
	
	//add fields
	uint row_num = 0;
	KexiDB::Field *field;
	for (KexiDB::Field::ListIterator it(*query->fields()); (field = it.current()); ++it, row_num++) {
		//add row
		QString tableName, fieldName, columnAlias;
		if (field->isQueryAsterisk()) {
			if (field->table()) {//single-table asterisk
				tableName = field->table()->name();
				fieldName = "*";
			}
			else {//all-tables asterisk
				tableName = "*";
				fieldName = "";
			}
		}
		else {
			columnAlias = query->columnAlias(row_num);
			if (field->isExpression()) {
//				if (columnAlias.isEmpty()) {
//					columnAlias = i18n("expression", "expr%1").arg(row_num); //TODO
//				}
				fieldName = field->expression()->toString();
			}
			else {
				tableName = field->table()->name();
				fieldName = field->name();
			}
		}
		KexiTableItem *newItem = createNewRow(tableName, fieldName); //, columnAlias);
		d->dataTable->tableView()->insertItem(newItem, row_num);
		//create buffer
		KexiPropertyBuffer &buf = *createPropertyBuffer( row_num, tableName, fieldName, true/*new one*/ );
		if (!columnAlias.isEmpty()) {//add alias
			buf["alias"].setValue(columnAlias, false);
		}
	}
	propertyBufferSwitched();

	if (!was_dirty)
		setDirty(false);
	d->dataTable->tableView()->ensureCellVisible(0,0);
}

bool KexiQueryDesignerGuiEditor::loadLayout()
{
	QString xml;
//	if (!loadDataBlock( xml, "query_layout" )) {
	loadDataBlock( xml, "query_layout" );
		//TODO errmsg
//		return false;
//	}
	if (xml.isEmpty()) {
		//in a case when query layout was not saved, build layout by hand
		showTablesAndConnectionsForQuery( static_cast<KexiDB::QuerySchema *>(parentDialog()->schemaData()) );
		return true;
	}

	QDomDocument doc;
	doc.setContent(xml);
	QDomElement doc_el = doc.documentElement(), el;
	if (doc_el.tagName()!="query_layout") {
		//TODO errmsg
		return false;
	}

	const bool was_dirty = dirty();

	//add tables and relations to the relation view
	for (el = doc_el.firstChild().toElement(); !el.isNull(); el=el.nextSibling().toElement()) {
		if (el.tagName()=="table") {
			KexiDB::TableSchema *t = d->conn->tableSchema(el.attribute("name"));
			int x = el.attribute("x","-1").toInt();
			int y = el.attribute("y","-1").toInt();
			int width = el.attribute("width","-1").toInt();
			int height = el.attribute("height","-1").toInt();
			QRect rect;
			if (x!=-1 || y!=-1 || width!=-1 || height!=-1)
				rect = QRect(x,y,width,height);
			d->relations->addTable( t, rect );
		}
		else if (el.tagName()=="conn") {
			SourceConnection src_conn;
			src_conn.masterTable = el.attribute("mtable");
			src_conn.masterField = el.attribute("mfield");
			src_conn.detailsTable = el.attribute("dtable");
			src_conn.detailsField = el.attribute("dfield");
			d->relations->addConnection(src_conn);
		}
	}

	if (!was_dirty)
		setDirty(false);
	return true;
}

bool KexiQueryDesignerGuiEditor::storeLayout()
{
	KexiQueryPart::TempData * temp = tempData();

	// Save SQL without driver-escaped keywords.
	QString sqlText = 
	  mainWin()->project()->dbConnection()->selectStatement( *temp->query, KexiDB::Driver::EscapeKexi|KexiDB::Driver::EscapeAsNecessary );
	if (!storeDataBlock( sqlText, "sql" )) {
		return false;
	}

	//serialize detailed XML query definition
	QString xml = "<query_layout>", tmp;
	for (TablesDictIterator it(*d->relations->tables()); it.current(); ++it) {
		KexiRelationViewTableContainer *table_cont = it.current();
		tmp = QString("<table name=\"")+table_cont->table()->name()+"\" x=\""
		 +QString::number(table_cont->x())
		 +"\" y=\""+QString::number(table_cont->y())
		 +"\" width=\""+QString::number(table_cont->width())
		 +"\" height=\""+QString::number(table_cont->height())
		 +"\"/>";
		xml += tmp;
	}

	KexiRelationViewConnection *con; 
	for (ConnectionListIterator it(*d->relations->connections()); (con = it.current()); ++it) {
		tmp = QString("<conn mtable=\"") + con->masterTable()->table()->name() 
			+ "\" mfield=\"" + con->masterField() + "\" dtable=\"" + con->detailsTable()->table()->name() 
			+ "\" dfield=\"" + con->detailsField() + "\"/>";
		xml += tmp;
	}
	xml += "</query_layout>";
	if (!storeDataBlock( xml, "query_layout" )) {
		return false;
	}
	return true;
}

QSize KexiQueryDesignerGuiEditor::sizeHint() const
{
	QSize s1 = d->relations->sizeHint();
	QSize s2 = d->head->sizeHint();
	return QSize(QMAX(s1.width(),s2.width()), s1.height()+s2.height());
}

#if 0
KexiTableItem* 
KexiQueryDesignerGuiEditor::createNewRow(const QString& tableName, const QString& fieldName) const
{
	return createNewRowInternal(tableName+"."+fieldName, tableName);
}

KexiTableItem* 
KexiQueryDesignerGuiEditor::createNewRow(const KexiDB::Field &exprField) const
{
	exprField.
	return createNewRowInternal(tableName+"."+fieldName, tableName);
}
#endif

KexiTableItem* 
KexiQueryDesignerGuiEditor::createNewRow(const QString& tableName, const QString& fieldName) const
{
	KexiTableItem *newItem = new KexiTableItem(d->data->columnsCount());
	QString key;
//	if (!alias.isEmpty())
//		key=alias+": ";
	if (tableName=="*")
		key="*";
	else {
		if (!tableName.isEmpty())
			key = (tableName+".");
		key += fieldName;
	}
	(*newItem)[0]=key;
	(*newItem)[1]=tableName;
	(*newItem)[2]=QVariant(true,1);//visible
	(*newItem)[3]=QVariant(0);//totals
	return newItem;
}

void KexiQueryDesignerGuiEditor::slotDragOverTableRow(KexiTableItem * /*item*/, int /*row*/, QDragMoveEvent* e)
{
	if (e->provides("kexi/field")) {
		e->acceptAction(true);
	}
}

void
KexiQueryDesignerGuiEditor::slotDroppedAtRow(KexiTableItem * /*item*/, int /*row*/, 
	QDropEvent *ev, KexiTableItem*& newItem)
{
	//TODO: better check later if the source is really a table
	QString srcTable;
	QString srcField;
	QString dummy;

	KexiFieldDrag::decode(ev,dummy,srcTable,srcField);
	//insert new row at specific place
	newItem = createNewRow(srcTable, srcField);
	d->droppedNewItem = newItem;
	d->droppedNewTable = srcTable;
	d->droppedNewField = srcField;
	//TODO
}

void KexiQueryDesignerGuiEditor::slotRowInserted(KexiTableItem* item, uint row, bool /*repaint*/)
{
	if (d->droppedNewItem && d->droppedNewItem==item) {
		createPropertyBuffer( row, d->droppedNewTable, d->droppedNewField, true );
		propertyBufferSwitched();
		d->droppedNewItem=0;
	}
}

void KexiQueryDesignerGuiEditor::slotTableAdded(KexiDB::TableSchema & /*t*/)
{
	if (!d->slotTableAdded_enabled)
		return;
	updateColumnsData();
	setDirty();
}

void KexiQueryDesignerGuiEditor::slotTableHidden(KexiDB::TableSchema & /*t*/)
{
	updateColumnsData();
	setDirty();
}

//! @todo this is primitive, temporary: reuse SQL parser
KexiDB::BaseExpr* KexiQueryDesignerGuiEditor::parseCriteriaString(
	const QString& columnName, const QString& fullString)
{
	QString str = fullString.stripWhiteSpace();
	int len = 0;
	KexiDB::BaseExpr *expr = 0;
	int token = 0;
	if (str.startsWith(">="))
		token = GREATER_OR_EQUAL;
	else if (str.startsWith("<="))
		token = LESS_OR_EQUAL;
	else if (str.startsWith("<>"))
		token = NOT_EQUAL;
	else if (str.startsWith("!="))
		token = NOT_EQUAL2;
	else if (str.startsWith("=="))
		token = '=';

	if (token!=0)
		len = 2;
	else if (str.startsWith("=")
		|| str.startsWith("<")
		|| str.startsWith(">"))
	{
		token = str[0].latin1();
		len = 1;
	}
	else {
		token = '=';
	}

	if (token==0)
		return 0;
	if (len>0)
		str = str.mid(len).stripWhiteSpace();
	if (str.isEmpty())
		return 0;

	KexiDB::BaseExpr *valueExpr = 0;
	QRegExp re;
	if (str.left(1)=="\"" || str.left(1)=="'") {
		if (str.length()<2 
			|| (str.left(1)=="\"" && str.right(1)!="\"")
			|| (str.left(1)=="'" && str.right(1)!="'"))
		{
			return 0;
		}
		valueExpr = new KexiDB::ConstExpr(CHARACTER_STRING_LITERAL, str.mid(1,str.length()-2));
	}
	else if ((re = QRegExp("(\\d{1,4})-(\\d{1,2})-(\\d{1,2})")).exactMatch( str ))
	{
			valueExpr = new KexiDB::ConstExpr(DATE_CONST, QDate::fromString(
				re.cap(1).rightJustify(4, '0')+"-"+re.cap(2).rightJustify(2, '0')
				+"-"+re.cap(3).rightJustify(2, '0'), Qt::ISODate));
	}
	else if ((re = QRegExp("(\\d{1,2}):(\\d{1,2})")).exactMatch( str )
	      || (re = QRegExp("(\\d{1,2}):(\\d{1,2}):(\\d{1,2})")).exactMatch( str ))
	{
		QString res = re.cap(1).rightJustify(2, '0')+":"+re.cap(2).rightJustify(2, '0')
			+":"+re.cap(3).rightJustify(2, '0');
//		kexipluginsdbg << res << endl;
		valueExpr = new KexiDB::ConstExpr(TIME_CONST, QTime::fromString(res, Qt::ISODate));
	}
	else if ((re = QRegExp("(\\d{1,4})-(\\d{1,2})-(\\d{1,2})\\s+(\\d{1,2}):(\\d{1,2})")).exactMatch( str )
	      || (re = QRegExp("(\\d{1,4})-(\\d{1,2})-(\\d{1,2})\\s+(\\d{1,2}):(\\d{1,2}):(\\d{1,2})")).exactMatch( str ))
	{
		QString res = re.cap(1).rightJustify(4, '0')+"-"+re.cap(2).rightJustify(2, '0')
			+"-"+re.cap(3).rightJustify(2, '0')
			+"T"+re.cap(4).rightJustify(2, '0')+":"+re.cap(5).rightJustify(2, '0')
			+":"+re.cap(6).rightJustify(2, '0');
//		kexipluginsdbg << res << endl;
		valueExpr = new KexiDB::ConstExpr(DATETIME_CONST,
			QDateTime::fromString(res, Qt::ISODate));
	}
	else if (str[0]>='0' && str[0]<='9' || str[0]=='-' || str[0]=='+') {
		//number
		QString decimalSym = KGlobal::locale()->decimalSymbol();
		const int pos = str.find(decimalSym);
		bool ok;
		if (pos>=0) {//real const number
			const int left = str.left(pos).toInt(&ok);
			if (!ok)
				return 0;
			const int right = str.mid(pos+1).toInt(&ok);
			if (!ok)
				return 0;
			valueExpr = new KexiDB::ConstExpr(REAL_CONST, QPoint(left,right)); //decoded to QPoint
		}
		else {
			//integer const
			const Q_LLONG val = str.toLongLong(&ok);
			if (!ok)
				return 0;
			valueExpr = new KexiDB::ConstExpr(INTEGER_CONST, val);
		}
	}
	else if (str.lower()=="null") {
		valueExpr = new KexiDB::ConstExpr(SQL_NULL, QVariant());
	}
	else {//identfier
		if (!Kexi::isIdentifier(str))
			return 0;
		valueExpr = new KexiDB::VariableExpr(str);
	}
	KexiDB::VariableExpr *varExpr = new KexiDB::VariableExpr(columnName);
	return new KexiDB::BinaryExpr(KexiDBExpr_Relational, varExpr, token, valueExpr);
}

void KexiQueryDesignerGuiEditor::slotBeforeCellChanged(KexiTableItem *item, int colnum, 
	QVariant newValue, KexiDB::ResultInfo* result)
{
	if (colnum==0) {//'field'
		if (newValue.isNull()) {
			d->data->updateRowEditBuffer(item, 1, QVariant(), false/*!allowSignals*/);
			d->data->updateRowEditBuffer(item, 2, QVariant(false,1));//invisible
			d->data->updateRowEditBuffer(item, 3, QVariant());//remove totals
			d->buffers->removeCurrentPropertyBuffer();
		}
		else {
			//auto fill 'table' column
			QString fieldName = newValue.toString();
			QString tableName = fieldName;
			const int id = tableName.find('.');
			if (id>=0)
				tableName = tableName.left(id);
			d->data->updateRowEditBuffer(item, 1, tableName, false/*!allowSignals*/);
			d->data->updateRowEditBuffer(item, 2, QVariant(true,1));//visible
			d->data->updateRowEditBuffer(item, 3, QVariant(0));//totals
			if (!propertyBuffer()) {
				createPropertyBuffer( d->dataTable->tableView()->currentRow(), 
					tableName, fieldName, true );
				propertyBufferSwitched();
			}
			//update property
			if (propertyBuffer()) {
				KexiPropertyBuffer &buf = *propertyBuffer();
				buf["field"] = fieldName.mid(id+1);
				buf["alias"] = QString::null;
				buf["caption"] = QString::null;
				buf["table"] = tableName;
			}
		}
	}
	else if (colnum==1) {//'table'
		if (newValue.isNull()) {
			if (!item->at(0).toString().isEmpty())
				d->data->updateRowEditBuffer(item, 0, QVariant(), false/*!allowSignals*/);
			d->data->updateRowEditBuffer(item, 2, QVariant(false,1));//invisible
			d->data->updateRowEditBuffer(item, 3, QVariant());//remove totals
			d->buffers->removeCurrentPropertyBuffer();
		}
		//update property
		if (propertyBuffer()) {
			KexiPropertyBuffer &buf = *propertyBuffer();
			buf["table"] = newValue;
			buf["caption"] = QString::null;
		}
	}
	else if (colnum==2) {//'visible'
		if (!propertyBuffer()) {
			createPropertyBuffer( d->dataTable->tableView()->currentRow(), 
				item->at(1).toString(), item->at(0).toString(), true );
			d->data->updateRowEditBuffer(item, 3, QVariant(0));//totals
			propertyBufferSwitched();
		}
		if (propertyBuffer()) {
			KexiPropertyBuffer &buf = *propertyBuffer();
			buf["visible"] = newValue;
		}
	}
	else if (colnum==3) {//'totals'
		//TODO:
		//unused yet
		setDirty(true);
	}
	else if (colnum==4) {//'criteria'
//TODO: this is primitive, temporary: reuse SQL parser
		QString operatorStr, argStr;
		KexiDB::BaseExpr* e = 0;
		const QString str = newValue.toString().stripWhiteSpace();
//		KexiPropertyBuffer &buf = *propertyBuffer();
		QString field, table;
		if (propertyBuffer()) {
			field = (*propertyBuffer())["field"].value().toString();
			table = (*propertyBuffer())["table"].value().toString();
		}
		if (!str.isEmpty() && (!propertyBuffer() || table=="*" || field.find("*")!=-1)) {
			//asterisk found! criteria not allowed
			result->success = false;
			result->column = 4;
			if (propertyBuffer())
				result->msg = i18n("Could not set criteria for \"%1\"")
					.arg(table=="*" ? table : field);
			else
				result->msg = i18n("Could not set criteria for empty row");
			d->dataTable->tableView()->cancelEditor(); //prevents further editing of this cell
		}
		else if (str.isEmpty() || (e = parseCriteriaString("whatevernotempty", str)))
		{
			//this is just checking: destroy expr. object
			delete e;
      setDirty(true);
			(*propertyBuffer())["criteria"] = str;
		} 
		else {
			result->success = false;
			result->column = 4;
			result->msg = i18n("Invalid criteria \"%1\"").arg(newValue.toString());
		}
	}
}

void KexiQueryDesignerGuiEditor::slotTablePositionChanged(KexiRelationViewTableContainer*)
{
	setDirty(true);
}

void KexiQueryDesignerGuiEditor::slotAboutConnectionRemove(KexiRelationViewConnection*)
{
	setDirty(true);
}

void KexiQueryDesignerGuiEditor::slotTableFieldDoubleClicked( KexiDB::TableSchema* table, const QString& fieldName )
{
	if (!table || (!table->field(fieldName) && fieldName!="*"))
		return;
	int row_num;
	//find last filled row in the GUI table
	for (row_num=d->buffers->size()-1; !d->buffers->at(row_num) && row_num>=0; row_num--)
		;
	row_num++; //after
	//add row
	KexiTableItem *newItem = createNewRow(table->name(), fieldName);
	d->dataTable->tableView()->insertItem(newItem, row_num);
	d->dataTable->tableView()->setCursor(row_num, 0);
	//create buffer
	createPropertyBuffer( row_num, table->name(), fieldName, true/*new one*/ );
	propertyBufferSwitched();
	d->dataTable->setFocus();
}

KexiPropertyBuffer *KexiQueryDesignerGuiEditor::propertyBuffer()
{
	return d->buffers->currentPropertyBuffer();
}

KexiPropertyBuffer *
KexiQueryDesignerGuiEditor::createPropertyBuffer( int row, 
	const QString& tableName, const QString& fieldName, bool newOne )
{
	const bool asterisk = (tableName=="*" || fieldName.find("*")!=-1);
	QString typeName = "KexiQueryDesignerGuiEditor::Column";
	KexiPropertyBuffer *buff = new KexiPropertyBuffer(d->buffers, typeName);

	KexiProperty *prop;
	buff->add(prop = new KexiProperty("table", QVariant(tableName)) );
	prop->setVisible(false);//always hidden

	buff->add(prop = new KexiProperty("field", QVariant(fieldName)) );
	prop->setVisible(false);//always hidden

	buff->add(prop = new KexiProperty("caption", QVariant(QString::null), i18n("Caption") ) );
	if (asterisk)
		prop->setVisible(false);
#ifdef KEXI_NO_UNFINISHED
		prop->setVisible(false);
#endif

	buff->add(prop = new KexiProperty("alias", QVariant(QString::null), i18n("Alias")) );
	if (asterisk)
		prop->setVisible(false);

	buff->add(prop = new KexiProperty("visible", QVariant(true, 4)) );
	prop->setVisible(false);

/*TODO: 
	buff->add(prop = new KexiProperty("totals", QVariant(QString::null)) );
	prop->setVisible(false);*/

	//sorting
	QStringList slist, nlist;
	slist << "nosorting" << "ascending" << "descending";
	nlist << i18n("None") << i18n("Ascending") << i18n("Descending");
	buff->add(prop = new KexiProperty("sorting", slist[0], slist, nlist, i18n("Sorting")));
	if (asterisk)
		prop->setVisible(false);

	buff->add(prop = new KexiProperty("criteria", QVariant(QString::null)) );
	prop->setVisible(false);

	connect(buff, SIGNAL(propertyChanged(KexiPropertyBuffer&, KexiProperty&)),
		this, SLOT(slotPropertyChanged(KexiPropertyBuffer&, KexiProperty&)));

	d->buffers->insert(row, buff, newOne);
	return buff;
}

void KexiQueryDesignerGuiEditor::setFocus()
{
	d->dataTable->setFocus();
}

void KexiQueryDesignerGuiEditor::slotPropertyChanged(KexiPropertyBuffer &buf, KexiProperty &property)
{
	const QCString& pname = property.name();
/*
 * TODO (js) use KexiProperty::setValidator(QString) when implemented as described in TODO #60 
 */
	if (pname=="alias" || pname=="name") {
		const QVariant& v = property.value();
		if (!v.toString().stripWhiteSpace().isEmpty() && !Kexi::isIdentifier( v.toString() )) {
			KMessageBox::sorry(this,
				Kexi::identifierExpectedMessage(property.desc(), v.toString()));
			property.resetValue();
		}
	}
}

#include "kexiquerydesignerguieditor.moc"

