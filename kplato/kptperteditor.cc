/* This file is part of the KDE project
  Copyright (C) 2007 Florian Piquemal <flotueur@yahoo.fr>
  Copyright (C) 2007 Alexis Ménard <darktears31@gmail.com>

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
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/
#include "kptperteditor.h"

namespace KPlato
{

void PertEditor::draw( Project &project)
{
    m_tasktree->clear();
    
    foreach(Node * currentNode, project.projectNode()->childNodeIterator()){
        if (currentNode->type()!=4){
            QTreeWidgetItem * item = new QTreeWidgetItem( m_tasktree );
            item->setText( 0, currentNode->name());
            drawSubTasksName(item,currentNode);
        }
        //kDebug() << "[void KPlato::PertEditor::draw( Project &project )] TASK FOUNDED" << endl;
    }

}

void PertEditor::drawSubTasksName( QTreeWidgetItem *parent, Node * currentNode)
{
    if (currentNode->numChildren() > 0){
        foreach(Node * currentChild, currentNode->childNodeIterator()){
            if (currentChild->type()!=4){
                QTreeWidgetItem * item = new QTreeWidgetItem( parent );
                item->setText( 0, currentChild->name());
                drawSubTasksName( item, currentChild);
            }
            //kDebug() << "[void KPlato::PertEditor::draw( Project &project )] SUBTASK FOUNDED" << endl;
        }
    }
}


//-----------------------------------
PertEditor::PertEditor( Part *part, QWidget *parent ) : ViewBase( part, parent )
{
    kDebug() << " ---------------- KPlato: Creating PertEditor ----------------" << endl;
    widget.setupUi(this);
    widget.assignList->setSelectedLabel(i18n("Required Tasks :"));
    widget.assignList->setAvailableLabel(i18n("Available Tasks :"));
    widget.assignList->setShowUpDownButtons(false);
    widget.assignList->layout()->setMargin(0);

    m_tasktree = widget.tableTaskWidget;
    m_assignList = widget.assignList;
    m_part = part;
    
    draw( m_part->getProject() );

    connect( m_tasktree, SIGNAL( itemSelectionChanged() ), SLOT( dispAvailableTasks() ) );
    connect( m_assignList, SIGNAL(added(QListWidgetItem *)), this, SLOT(addTaskInRequiredList(QListWidgetItem * )) );
    connect( m_assignList, SIGNAL(removed(QListWidgetItem *)), this, SLOT(removeTaskFromRequiredList(QListWidgetItem * )) );
}

void PertEditor::dispAvailableTasks(){
    QString selectedTaskName = m_tasktree->selectedItems().first()->text(0);

    m_assignList->availableListWidget()->clear();
    m_assignList->selectedListWidget()->clear();

    loadRequiredTasksList(itemToNode(selectedTaskName));

    foreach(Node * currentNode, m_part->getProject().projectNode()->childNodeIterator() ){
        // Checks if the curent node is not a milestone
        // and if it isn't the same as the selected task in the m_tasktree
        if ( currentNode->type() != 4 and currentNode->name() != selectedTaskName
               and (m_assignList->selectedListWidget()->findItems(currentNode->name(),0)).empty()){
            m_assignList->availableListWidget()->addItem(currentNode->name());
        }
    }
}

Node * PertEditor::itemToNode(QString itemName){
    Node * result;
    foreach(Node * currentNode, m_part->getProject().projectNode()->childNodeIterator() ){
        if (currentNode->name() == itemName) {
            result=currentNode;
        }
    }
    return result;
}

void PertEditor::addTaskInRequiredList(QListWidgetItem * currentItem){
    // add the selected task to the RequiredTasksList 
    QString selectedTaskName = m_tasktree->selectedItems().first()->text(0);

    static_cast<Task *>(itemToNode(selectedTaskName))->addRequiredTask(itemToNode(currentItem->text()));

}

void PertEditor::removeTaskFromRequiredList(QListWidgetItem * currentItem){
    // remove the selected task from the RequiredTasksList
    QString selectedTaskName = m_tasktree->selectedItems().first()->text(0);

    static_cast<Task *>(itemToNode(selectedTaskName))->remRequiredTask(itemToNode(currentItem->text()));

}

void PertEditor::loadRequiredTasksList(Node * taskNode){
    // Display the required task list into the rigth side of m_assignList
    m_assignList->selectedListWidget()->clear();

    foreach(Node * currentNode, static_cast<Task *>(taskNode)->requiredTaskIterator()){
        m_assignList->selectedListWidget()->addItem(currentNode->name());
    }
}


} // namespace KPlato

#include "kptperteditor.moc"
