/******************************************************************/
/* KPresenter - (c) by Reginald Stadlbauer 1997-1998              */
/* Version: 0.1.0alpha                                            */
/* Author: Reginald Stadlbauer                                    */
/* E-Mail: reggie@kde.org                                         */
/* Homepage: http://boch35.kfunigraz.ac.at/~rs                    */
/* needs c++ library Qt (http://www.troll.no)                     */
/* needs mico (http://diamant.vsb.cs.uni-frankfurt.de/~mico/)     */
/* needs OpenParts and Kom (weis@kde.org)                         */
/* written for KDE (http://www.kde.org)                           */
/* License: GNU GPL                                               */
/******************************************************************/
/* Module: Background Dialog                                      */
/******************************************************************/

#include <kfiledialog.h>

#include "backdia.h"
#include "backdia.moc"

/******************************************************************/
/* class ClipPreview                                              */
/******************************************************************/

/*======================= constructor ============================*/
ClipPreview::ClipPreview(QWidget* parent=0,const char* name=0)
  : QWidget(parent,name)
{
  setBackgroundColor(white);
  pic = 0;
  pic = new QPicture;
}

/*======================= destructor =============================*/
ClipPreview::~ClipPreview()
{
  delete pic;
}

/*==================== set clipart ===============================*/
void ClipPreview::setClipart(const char* fn)
{
  fileName = qstrdup(fn);
  wmf.load(fileName);
  wmf.paint(pic);
  repaint();
}

/*===================== paint ====================================*/
void ClipPreview::paintEvent(QPaintEvent*)
{
  QPainter p;
 
  p.begin(this);
  pic->play(&p);
  p.end();
}

/******************************************************************/
/* class BackDia                                                  */
/******************************************************************/

/*==================== constructor ===============================*/
BackDia::BackDia(QWidget* parent=0,const char* name=0,BackType backType=BT_COLOR,
		 QColor backColor1=white,QColor backColor2=white,BCType _bcType=BCT_PLAIN,
		 const char *backPic=0,const char *backClip=0,
		 BackView backPicView=BV_TILED)
  :QDialog(parent,name,true)
{
  int col1 = 20,col2;
  int row1 = 15,row2,butW,butH;

  lPicName = 0;
  lClipName = 0;

  buttGrp = new QButtonGroup(this,"bgrp1");
  buttGrp->setFrameStyle(QFrame::NoFrame);
  buttGrp2 = new QButtonGroup(this,"bgrp2");
  buttGrp2->setFrameStyle(QFrame::NoFrame);
  buttGrp3 = new QButtonGroup(this,"bgrp3");
  buttGrp3->setFrameStyle(QFrame::NoFrame);

  radioColor = new QRadioButton("Color",this,"radioColor");
  radioColor->move(col1,row1);
  radioColor->resize(radioColor->sizeHint());
  if (backType == BT_COLOR) radioColor->setChecked(true);
  buttGrp->insert(radioColor);

  grp1 = new QGroupBox(this);
  grp1->move(radioColor->x(),radioColor->y()+radioColor->height()+20);
  
  color1Choose = new KColorButton(backColor1,grp1);
  color1Choose->move(10,10);
  color1Choose->resize(color1Choose->sizeHint());

  color2Choose = new KColorButton(backColor2,grp1);
  color2Choose->move(10,color1Choose->y()+color1Choose->height()+10);
  color2Choose->resize(color2Choose->sizeHint());
  

  grp1->resize(2*color1Choose->x()+color1Choose->width(),
	       2*color1Choose->y()+20+color1Choose->height()+color2Choose->height());

  radioPic = new QRadioButton("Picture (Pixel-Graphic)",this,"radioPic");
  radioPic->move(grp1->x()+grp1->width()+20,radioColor->y());
  radioPic->resize(radioPic->sizeHint());
  if (backType == BT_PIC) radioPic->setChecked(true);
  buttGrp->insert(radioPic);

  grp2 = new QGroupBox(this);
  grp2->move(grp1->x()+grp1->width()+20,grp1->y());

  vTiled = new QRadioButton("Tiled",grp2,"tiled");
  vTiled->move(10,10);
  vTiled->resize(vTiled->sizeHint());
  if (backPicView == BV_TILED) vTiled->setChecked(true);
  buttGrp2->insert(vTiled);

  vCenter = new QRadioButton("Center",grp2,"center");
  vCenter->move(vTiled->x()+vTiled->width()+10,10);
  vCenter->resize(vCenter->sizeHint());
  if (backPicView == BV_CENTER) vCenter->setChecked(true);
  buttGrp2->insert(vCenter);

  vZoom = new QRadioButton("Zoom",grp2,"zoom");
  vZoom->move(vCenter->x()+vCenter->width()+10,10);
  vZoom->resize(vZoom->sizeHint());
  if (backPicView == BV_ZOOM) vZoom->setChecked(true);
  buttGrp2->insert(vZoom);  

  picChoose = new QPushButton("Choose Picture...",grp2,"picChoose");
  picChoose->setGeometry(10,vTiled->y()+vTiled->height()+20,
			 vTiled->width()+vCenter->width()+vZoom->width()+20,
			 picChoose->sizeHint().height());
  connect(picChoose,SIGNAL(clicked()),this,SLOT(selectPic()));

  picPreview = new QLabel(grp2,"picPre");
  picPreview->setBackgroundColor(white);
  picPreview->setGeometry(10,picChoose->y()+picChoose->height()+20,picChoose->width(),180);
  if (backPic) openPic(qstrdup(backPic));
  if (backPic) chosenPic = qstrdup(backPic);
  else chosenPic = 0;
 
  lPicName = new QLabel(grp2,"picname");
  if (backPic) lPicName->setText(qstrdup(backPic));
  else lPicName->setText("no picture");
  lPicName->setGeometry(10,picPreview->y()+picPreview->height()+20,
			picChoose->width(),lPicName->sizeHint().height());

  grp2->resize(2*picChoose->x()+picChoose->width(),
	       lPicName->y()+lPicName->height()+10);

  radioClip = new QRadioButton("Clipart (Vector-Graphic)",this,"radioClip");
  radioClip->move(grp2->x()+grp2->width()+20,radioColor->y());
  radioClip->resize(radioClip->sizeHint());
  if (backType == BT_CLIP) radioClip->setChecked(true);
  buttGrp->insert(radioClip);

  grp3 = new QGroupBox(this);
  grp3->move(grp2->x()+grp2->width()+20,grp2->y());
  grp3->resize(grp2->width(),grp2->height());

  clipChoose = new QPushButton("Choose Clipart...",grp3,"clipChoose");
  clipChoose->setGeometry(10,10,grp3->width()-20,clipChoose->sizeHint().height());
  connect(clipChoose,SIGNAL(clicked()),this,SLOT(selectClip()));

  clipPreview = new ClipPreview(grp3); 
  clipPreview->move(10,clipChoose->y()+clipChoose->height()+20);
  clipPreview->resize(grp3->width()-20,grp3->width()-20);
  if (backClip) openClip(qstrdup(backClip));
  if (backClip) chosenClip = qstrdup(backClip);
  else chosenClip = 0;
  
  lClipName = new QLabel(grp3,"clipname");
  if (backClip) lClipName->setText(qstrdup(backClip));
  else lClipName->setText("no clipart");
  lClipName->setGeometry(10,clipPreview->y()+clipPreview->height()+20,
			 clipChoose->width(),lClipName->sizeHint().height());
  
  grp3->resize(grp2->width(),lClipName->height()+lClipName->y()+20);

  col2 = grp3->x()+grp3->width();
  row2 = grp2->y()+grp2->height()+20;

  cancelBut = new QPushButton(this,"BCancel");
  cancelBut->setText("Cancel");
 
  applyBut = new QPushButton(this,"BApply");
  applyBut->setText("Apply");

  applyGlobalBut = new QPushButton(this,"BApplyGlobal");
  applyGlobalBut->setText("Apply Global");

  okBut = new QPushButton(this,"BOK");
  okBut->setText("OK");
  okBut->setAutoRepeat(false);
  okBut->setAutoResize(false);
  okBut->setAutoDefault(true);
  okBut->setDefault(true);

  butW = max(cancelBut->sizeHint().width(),
	     max(applyBut->sizeHint().width(),okBut->sizeHint().width()));
  butH = cancelBut->sizeHint().height();

  cancelBut->resize(butW,butH);
  applyBut->resize(butW,butH);
  applyGlobalBut->resize(applyGlobalBut->sizeHint().width(),butH);
  okBut->resize(butW,butH);

  cancelBut->move(col2-cancelBut->width(),row2);
  applyBut->move(cancelBut->x()-5-applyBut->width(),row2);
  applyGlobalBut->move(applyBut->x()-5-applyGlobalBut->width(),row2);
  okBut->move(applyGlobalBut->x()-10-okBut->width(),row2);

  connect(okBut,SIGNAL(clicked()),this,SLOT(Ok()));
  connect(applyBut,SIGNAL(clicked()),this,SLOT(Apply()));
  connect(applyGlobalBut,SIGNAL(clicked()),this,SLOT(ApplyGlobal()));
  connect(cancelBut,SIGNAL(clicked()),this,SLOT(reject()));
  connect(okBut,SIGNAL(clicked()),this,SLOT(accept()));

  resize(grp3->x()+grp3->width()+20,row2+butH+10);
}

/*===================== destructor ===============================*/
BackDia::~BackDia()
{
  delete color1Choose;
  delete color2Choose;
  delete radioColor;
  delete picPreview;
  delete picChoose;
  delete radioPic;
  delete vTiled;
  delete vZoom;
  delete vCenter;
  delete lPicName;
  delete radioClip;
  delete clipChoose;
  delete clipPreview;
  delete cancelBut;
  delete applyBut;
  delete applyGlobalBut;
  delete okBut;
  delete grp2;
  delete grp1;
  delete grp3;
}

/*===================== get background type ======================*/
BackType BackDia::getBackType()
{
  if (radioColor->isChecked()) return BT_COLOR;
  if (radioPic->isChecked()) return BT_PIC;
  if (radioClip->isChecked()) return BT_CLIP;
  return BT_COLOR;
}

/*===================== get background pic view ==================*/
BackView BackDia::getBPicView()
{
  if (vTiled->isChecked()) return BV_TILED;
  if (vCenter->isChecked()) return BV_CENTER;
  if (vZoom->isChecked()) return BV_ZOOM;
  return BV_TILED;
}

/*=================== choose a picture ===========================*/
void BackDia::selectPic()
{
  radioPic->setChecked(true);
  radioColor->setChecked(false);
  radioClip->setChecked(false);

  QString file = KFileDialog::getOpenFileName(getenv("HOME"),"*.gif ; *.jpg ; *.jpeg ; *.bmp ; *.xbm ; *.xpm ; *.pnm ; *.GIF ; *.JPG ; *.JPEG ; *.BMP ; *.XBM ; *.XPM ; *.PNM",0);
  if (!file.isEmpty()) openPic((const char*)file);
}

/*=================== choose a clipart ===========================*/
void BackDia::selectClip()
{
  radioClip->setChecked(true);
  radioColor->setChecked(false);
  radioPic->setChecked(false);

  QString file = KFileDialog::getOpenFileName(getenv("HOME"),"*.wmf ; *.WMF",0);
  if (!file.isEmpty()) openClip((const char*)file);
}

/*==================== open a picture ============================*/
void BackDia::openPic(const char *picName)
{
  QApplication::setOverrideCursor(waitCursor);
  
  chosenPic = qstrdup(picName);
  QPixmap pix(picName);
  QWMatrix m;

  if (!pix.isNull())
    {
      m.scale((float)picPreview->width()/pix.width(),
	      (float)picPreview->height()/pix.height());
      picPreview->setPixmap(pix.xForm(m));
      if (lPicName) lPicName->setText(chosenPic);
    }
  QApplication::restoreOverrideCursor();
}

/*==================== open a clipart ============================*/
void BackDia::openClip(const char *clipName)
{
  QApplication::setOverrideCursor(waitCursor);
  chosenClip = qstrdup(clipName);
  clipPreview->setClipart(clipName);
  if (lClipName) lClipName->setText(chosenClip);
  QApplication::restoreOverrideCursor();
}
      
