// using namespace KPlato;

void KPTTaskGeneralPanelBase::setSchedulingType(int type)
{
    switch (type)
    {
    case 0:
    case 1:
	schedulerDate->setEnabled(false);
	schedulerTime->setEnabled(false);
	specifyTime->setEnabled(false);
	break;
    default:
	schedulerDate->setEnabled(true);
	schedulerTime->setEnabled(true);
	specifyTime->setEnabled(true);
	break;
    }
    scheduleType->setButton(type);
    emit schedulingTypeChanged(type);
    checkAllFieldsFilled();
}

void KPTTaskGeneralPanelBase::setDateTime( const QDateTime &dt )
{
    schedulerDate->setDate(dt.date());
    schedulerTime->setTime(dt.time());
}

QDateTime KPTTaskGeneralPanelBase::dateTime() const
{
    return QDateTime(schedulerDate->getDate(), schedulerTime->time());
}

int KPTTaskGeneralPanelBase::schedulingType() const 
{
    return scheduleType->selectedId();
}

void KPTTaskGeneralPanelBase::init()
{
}

void KPTTaskGeneralPanelBase::checkAllFieldsFilled()
{
    emit obligatedFieldsFilled( !(namefield->text().isEmpty() || idfield->text().isEmpty()));
}

void KPTTaskGeneralPanelBase::changeLeader()
{
    KABC::Addressee a = KABC::AddresseeDialog::getAddressee(this);
    if (!a.isEmpty())
    {
        leaderfield->setText(a.fullEmail());
    }
}
