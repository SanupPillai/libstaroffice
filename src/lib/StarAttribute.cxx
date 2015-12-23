/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libstaroffice
* Version: MPL 2.0 / LGPLv2+
*
* The contents of this file are subject to the Mozilla Public License Version
* 2.0 (the "License"); you may not use this file except in compliance with
* the License or as specified alternatively below. You may obtain a copy of
* the License at http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Major Contributor(s):
* Copyright (C) 2002 William Lachance (wrlach@gmail.com)
* Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
* Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
* Copyright (C) 2006, 2007 Andrew Ziem
* Copyright (C) 2011, 2012 Alonso Laurent (alonso@loria.fr)
*
*
* All Rights Reserved.
*
* For minor contributions see the git repository.
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
* in which case the provisions of the LGPLv2+ are applicable
* instead of those above.
*/

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFGraphicStyle.hxx"

#include "SWFieldManager.hxx"
#include "StarFormatManager.hxx"

#include "StarBitmap.hxx"
#include "StarCellAttribute.hxx"
#include "StarCharAttribute.hxx"
#include "StarGraphicAttribute.hxx"
#include "StarParagraphAttribute.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallText.hxx"
#include "StarObjectText.hxx"
#include "StarZone.hxx"

#include "StarAttribute.hxx"

/** Internal: the structures of a StarAttribute */
namespace StarAttributeInternal
{
//! xml attribute of StarAttributeInternal
class StarAttributeXML : public StarAttributeVoid
{
  // call SfxPoolItem::Create which does nothing
public:
  //! constructor
  StarAttributeXML(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarAttributeXML(*this));
  }
};

////////////////////////////////////////
//! Internal: the state of a StarAttribute
struct State {
  //! constructor
  State() : m_whichToAttributeMap()
  {
    initAttributeMap();
  }
  //! init the attribute map list
  void initAttributeMap();
  //! a map which to an attribute
  std::map<int, shared_ptr<StarAttribute> > m_whichToAttributeMap;
protected:
  //! add a void attribute
  void addAttributeVoid(StarAttribute::Type type, std::string const &debugName)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeVoid(type,debugName));
  }
  //! add a XML attribute
  void addAttributeXML(StarAttribute::Type type, std::string const &debugName)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeXML(type,debugName));
  }
  //! add a bool attribute
  void addAttributeBool(StarAttribute::Type type, std::string const &debugName, bool defValue)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeBool(type,debugName, defValue));
  }
  //! add a int attribute
  void addAttributeInt(StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeInt(type,debugName, numBytes, defValue));
  }
  //! add a unsigned int attribute
  void addAttributeUInt(StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeUInt(type,debugName, numBytes, defValue));
  }
  //! add a double attribute
  void addAttributeDouble(StarAttribute::Type type, std::string const &debugName, double defValue)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeDouble(type,debugName, defValue));
  }
  //! add a color attribute
  void addAttributeColor(StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeColor(type,debugName, defValue));
  }
  //! add a itemSet attribute
  void addAttributeItemSet(StarAttribute::Type type, std::string const &debugName, std::vector<STOFFVec2i> const &limits)
  {
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeItemSet(type,debugName, limits));
  }
};

void State::initAttributeMap()
{
  StarCellAttribute::addInitTo(m_whichToAttributeMap);
  StarCharAttribute::addInitTo(m_whichToAttributeMap);
  StarGraphicAttribute::addInitTo(m_whichToAttributeMap);
  StarParagraphAttribute::addInitTo(m_whichToAttributeMap);

  std::stringstream s;

  // --- sw --- sw_init.cxx
  addAttributeBool(StarAttribute::ATTR_PARA_SPLIT,"para[split]",true);
  addAttributeUInt(StarAttribute::ATTR_PARA_WIDOWS,"para[widows]",1,0); // numlines
  addAttributeUInt(StarAttribute::ATTR_PARA_ORPHANS,"para[orphans]",1,0); // numlines
  addAttributeBool(StarAttribute::ATTR_PARA_REGISTER,"para[register]",false);
  addAttributeBool(StarAttribute::ATTR_PARA_SCRIPTSPACE,"para[scriptSpace]",false);
  addAttributeBool(StarAttribute::ATTR_PARA_HANGINGPUNCTUATION,"para[hangingPunctuation]",true);
  addAttributeBool(StarAttribute::ATTR_PARA_FORBIDDEN_RULES,"para[forbiddenRules]",true);
  addAttributeUInt(StarAttribute::ATTR_PARA_VERTALIGN,"para[vert,align]",2,0);
  addAttributeBool(StarAttribute::ATTR_PARA_SNAPTOGRID,"para[snapToGrid]",true);
  addAttributeBool(StarAttribute::ATTR_PARA_CONNECT_BORDER,"para[connectBorder]",true);
  for (int type=StarAttribute::ATTR_PARA_DUMMY5; type<=StarAttribute::ATTR_PARA_DUMMY8; ++type) {
    s.str("");
    s << "paraDummy" << type-StarAttribute::ATTR_PARA_DUMMY5+5;
    addAttributeBool(StarAttribute::Type(type), s.str(), false);
  }

  addAttributeUInt(StarAttribute::ATTR_FRM_FILL_ORDER,"frm[fill,order]",1,0); // topDown
  addAttributeUInt(StarAttribute::ATTR_FRM_PAPER_BIN,"frm[paperBin]",1,0xFF); // settings
  addAttributeBool(StarAttribute::ATTR_FRM_PRINT,"print",true);
  addAttributeBool(StarAttribute::ATTR_FRM_OPAQUE,"opaque",true);
  addAttributeBool(StarAttribute::ATTR_FRM_KEEP,"frm[keep]",false);
  addAttributeBool(StarAttribute::ATTR_FRM_EDIT_IN_READONLY,"edit[readOnly]",false);
  addAttributeBool(StarAttribute::ATTR_FRM_LAYOUT_SPLIT,"layout[split]", true);
  addAttributeBool(StarAttribute::ATTR_FRM_COLUMNBALANCE,"col[noBalanced]", true);
  addAttributeUInt(StarAttribute::ATTR_FRM_FRAMEDIR,"frame[dir]",2,4); // frame environment
  addAttributeBool(StarAttribute::ATTR_FRM_HEADER_FOOTER_EAT_SPACING,"headFoot[eat,spacing]",false);
  addAttributeBool(StarAttribute::ATTR_FRM_FRMATTR_DUMMY9,"grfDummy9", false);

  addAttributeInt(StarAttribute::ATTR_GRF_ROTATION,"grf[rotation]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_LUMINANCE,"grf[luminance]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CONTRAST,"grf[contrast]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CHANNELR,"grf[channelR]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CHANNELG,"grf[channelG]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CHANNELB,"grf[channelB]",2,0);
  addAttributeDouble(StarAttribute::ATTR_GRF_GAMMA,"grf[gamma]",1);
  addAttributeBool(StarAttribute::ATTR_GRF_INVERT,"grf[invert]", false);
  addAttributeUInt(StarAttribute::ATTR_GRF_TRANSPARENCY,"grf[transparency]",1,0);
  addAttributeUInt(StarAttribute::ATTR_GRF_DRAWMODE,"grf[draw,mode]",2,0);
  for (int type=StarAttribute::ATTR_GRF_DUMMY1; type<=StarAttribute::ATTR_GRF_DUMMY5; ++type) {
    s.str("");
    s << "grafDummy" << type-StarAttribute::ATTR_GRF_DUMMY1+1;
    addAttributeBool(StarAttribute::Type(type), s.str(), false);
  }
  // --- ee --- svx_editdoc.cxx and svx_eerdll.cxx
  addAttributeVoid(StarAttribute::ATTR_EE_CHR_RUBI_DUMMY, "chr[rubi,dummy]");
  addAttributeXML(StarAttribute::ATTR_EE_CHR_XMLATTRIBS, "chr[xmlAttrib]");
  addAttributeUInt(StarAttribute::ATTR_EE_PARA_BULLETSTATE,"para[bullet,state]",2,0);
  addAttributeUInt(StarAttribute::ATTR_EE_PARA_OUTLLEVEL,"para[outlevel]",2,0);
  addAttributeBool(StarAttribute::ATTR_EE_PARA_ASIANCJKSPACING,"para[asianCJKSpacing]",false);
  addAttributeXML(StarAttribute::ATTR_EE_PARA_XMLATTRIBS, "para[xmlAttrib]");
  addAttributeVoid(StarAttribute::ATTR_EE_FEATURE_TAB, "feature[tab]");
  addAttributeVoid(StarAttribute::ATTR_EE_FEATURE_LINEBR, "feature[linebr]");
  // --- sch --- sch_itempool.cxx
  addAttributeBool(StarAttribute::ATTR_SCH_DATADESCR_SHOW_SYM,"dataDescr[showSym]", false);
  addAttributeUInt(StarAttribute::ATTR_SCH_DATADESCR_DESCR,"dataDescr[descr]",2,0); // none
  addAttributeUInt(StarAttribute::ATTR_SCH_LEGEND_POS,"legend[pos]",2,3); // right
  addAttributeUInt(StarAttribute::ATTR_SCH_TEXT_ORIENT,"text[orient]",2,1); // standard
  addAttributeUInt(StarAttribute::ATTR_SCH_TEXT_ORDER,"text[order]",2,0); // side by side

  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_MIN,"xAxis[autoMin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MIN,"yAxis[autoMin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MIN,"zAxis[autoMin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_MIN,"axis[autoMin]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_MIN,"xAxis[min]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_MIN,"yAxis[min]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_MIN,"zAxis[min]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_MIN,"axis[min]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_MAX,"xAxis[autoMax]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MAX,"yAxis[autoMax]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MAX,"zAxis[autoMax]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_MAX,"axis[autoMax]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_MAX,"xAxis[max]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_MAX,"yAxis[max]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_MAX,"zAxis[max]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_MAX,"axis[max]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_MAIN,"xAxis[autoStepMain]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_MAIN,"yAxis[autoStepMain]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_MAIN,"zAxis[autoStepMain]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_MAIN,"axis[autoStepMain]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_STEP_MAIN,"xAxis[step,main]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_STEP_MAIN,"yAxis[step,main]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_STEP_MAIN,"zAxis[step,main]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_STEP_MAIN,"axis[step,main]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_HELP,"xAxis[autoStepHelp]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_HELP,"yAxis[autoStepHelp]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_HELP,"zAxis[autoStepHelp]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_HELP,"axis[autoStepHelp]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_STEP_HELP,"xAxis[step,help]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_STEP_HELP,"yAxis[step,help]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_STEP_HELP,"zAxis[step,help]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_STEP_HELP,"axis[step,help]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_LOGARITHM,"xAxis[logarithm]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_LOGARITHM,"yAxis[logarithm]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_LOGARITHM,"zAxis[logarithm]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_LOGARITHM,"axis[logarithm]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_ORIGIN,"xAxis[origin]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_ORIGIN,"yAxis[origin]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_ORIGIN,"zAxis[origin]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_ORIGIN,"axis[origin]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_ORIGIN,"xAxis[autoOrigin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_ORIGIN,"yAxis[autoOrigin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_ORIGIN,"zAxis[autoOrigin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_ORIGIN,"axis[autoOrigin]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_AXISTYPE, "axis[type]",4, 0); // XAxis
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY0, "sch[dummy0]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY1, "sch[dummy1]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY2, "sch[dummy2]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY3, "sch[dummy3]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY_END, "sch[dummy4]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_STAT_KIND_ERROR, "stat[error,kind]",4, 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_PERCENT,"stat[percent]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_BIGERROR,"stat[big,error]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_CONSTPLUS,"stat[const,plus]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_CONSTMINUS,"stat[const,minus]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_STAT_AVERAGE,"stat[average]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_STAT_REGRESSTYPE, "stat[regress,type]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_STAT_INDICATE, "stat[indicate]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DEGREES, "text[degrees]",4, 0);
  addAttributeBool(StarAttribute::ATTR_SCH_TEXT_OVERLAP,"text[overlap]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY0, "text[dummy0]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY1, "text[dummy1]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY2, "text[dummy2]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY3, "text[dummy3]",4, 0);

  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_DEEP,"style[deep]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_3D,"style[3d]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_VERTICAL,"style[vertical]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_BASETYPE, "style[base,type]",4, 0);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_LINES,"style[lines]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_PERCENT,"style[percent]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_STACKED,"style[stacked]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_SPLINES, "style[splines]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_SYMBOL, "style[symbol]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_SHAPE, "style[shape]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_AXIS, "axis",4, 2);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_TICKS, "axis[ticks]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_HELPTICKS, "axis[help,ticks]",4, 2);
  addAttributeUInt(StarAttribute::ATTR_SCH_AXIS_NUMFMT,"axis[numFmt]",4,0);
  addAttributeUInt(StarAttribute::ATTR_SCH_AXIS_NUMFMTPERCENT,"axis[numFmt,percent]",4,11);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWAXIS,"axis[show]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWDESCR,"axis[showDescr]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWMAINGRID,"axis[show,mainGrid]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWHELPGRID,"axis[show,helpGrid]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_TOPDOWN,"axis[topDown]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY0, "axis[dummy0]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY1, "axis[dummy1]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY2, "axis[dummy2]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY3, "axis[dummy3]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_BAR_OVERLAP, "bar[overlap]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_BAR_GAPWIDTH, "bar[gap,width]",4, 0);

  addAttributeBool(StarAttribute::ATTR_SCH_STOCK_VOLUME,"stock[volume]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STOCK_UPDOWN,"stock[updown]", false);
  addAttributeXML(StarAttribute::ATTR_SCH_USER_DEFINED_ATTR,"sch[userDefined]");
  // --- xattr --- svx_xpool.cxx
  addAttributeUInt(StarAttribute::XATTR_LINESTYLE,"line[style]",2,1); // solid
  addAttributeInt(StarAttribute::XATTR_LINEWIDTH, "line[width]",4, 0); // metric
  addAttributeInt(StarAttribute::XATTR_LINESTARTWIDTH, "line[start,width]",4, 200); // metric
  addAttributeInt(StarAttribute::XATTR_LINEENDWIDTH, "line[end,width]",4, 200); // metric
  addAttributeBool(StarAttribute::XATTR_LINESTARTCENTER,"line[startCenter]", false);
  addAttributeBool(StarAttribute::XATTR_LINEENDCENTER,"line[endCenter]", false);
  addAttributeUInt(StarAttribute::XATTR_LINETRANSPARENCE,"line[transparence]",2,0);
  addAttributeUInt(StarAttribute::XATTR_LINEJOINT,"line[joint]",2,4); // use arc
  for (int type=StarAttribute::XATTR_LINERESERVED2; type<=StarAttribute::XATTR_LINERESERVED_LAST; ++type) {
    s.str("");
    s << "line[reserved" << type-StarAttribute::XATTR_LINERESERVED2+2 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeUInt(StarAttribute::XATTR_FILLSTYLE,"fill[style]",2,1); // solid
  addAttributeUInt(StarAttribute::XATTR_FILLTRANSPARENCE,"fill[transparence]",2,0);
  addAttributeUInt(StarAttribute::XATTR_GRADIENTSTEPCOUNT,"gradient[stepCount]",2,0);
  addAttributeBool(StarAttribute::XATTR_FILLBMP_TILE,"fill[bmp,tile]", true);
  addAttributeBool(StarAttribute::XATTR_FILLBMP_STRETCH,"fill[bmp,stretch]", true);
  addAttributeBool(StarAttribute::XATTR_FILLBMP_SIZELOG,"fill[bmp,sizeLog]", true);
  addAttributeBool(StarAttribute::XATTR_FILLBACKGROUND,"fill[background]", false);
  addAttributeUInt(StarAttribute::XATTR_FILLBMP_POS,"fill[bmp,pos]",2,4); // middle-middle
  addAttributeInt(StarAttribute::XATTR_FILLBMP_SIZEX, "fill[bmp,sizeX]",4, 0); // metric
  addAttributeInt(StarAttribute::XATTR_FILLBMP_SIZEY, "fill[bmp,sizeY]",4, 0); // metric
  addAttributeUInt(StarAttribute::XATTR_FILLBMP_TILEOFFSETX,"fill[bmp,tile,offX]",2,0);
  addAttributeUInt(StarAttribute::XATTR_FILLBMP_TILEOFFSETY,"fill[bmp,tile,offY]",2,0);
  addAttributeUInt(StarAttribute::XATTR_FILLBMP_POSOFFSETX,"fill[bmp,pos,offX]",2,0);
  addAttributeUInt(StarAttribute::XATTR_FILLBMP_POSOFFSETY,"fill[bmp,pos,offY]",2,0);
  for (int type=StarAttribute::XATTR_FILLRESERVED2; type<=StarAttribute::XATTR_FILLRESERVED_LAST; ++type) {
    s.str("");
    s << "fill[reserved" << type-StarAttribute::XATTR_FILLRESERVED2+2 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeUInt(StarAttribute::XATTR_FORMTXTSTYLE,"formText[style]",2,4); // None
  addAttributeUInt(StarAttribute::XATTR_FORMTXTADJUST,"formText[adjust]",2,3); // Adjust
  addAttributeInt(StarAttribute::XATTR_FORMTXTDISTANCE, "formText[distance]",4, 0); // metric
  addAttributeInt(StarAttribute::XATTR_FORMTXTSTART, "formText[start]",4, 0); // metric
  addAttributeBool(StarAttribute::XATTR_FORMTXTMIRROR,"formText[mirror]", false);
  addAttributeBool(StarAttribute::XATTR_FORMTXTOUTLINE,"formText[outline]", false);
  addAttributeUInt(StarAttribute::XATTR_FORMTXTSHADOW,"formText[shadow]",2,0); // None
  addAttributeInt(StarAttribute::XATTR_FORMTXTSHDWXVAL, "formText[shadow,xDist]",4, 0); // metric
  addAttributeInt(StarAttribute::XATTR_FORMTXTSHDWYVAL, "formText[shadow,yDist]",4, 0); // metric
  addAttributeUInt(StarAttribute::XATTR_FORMTXTSTDFORM,"formText[stdForm]",2,0); // None
  addAttributeBool(StarAttribute::XATTR_FORMTXTHIDEFORM,"formText[hide]", false);
  addAttributeUInt(StarAttribute::XATTR_FORMTXTSHDWTRANSP,"formText[shadow,trans]",2,0);
  for (int type=StarAttribute::XATTR_FTRESERVED2; type<=StarAttribute::XATTR_FTRESERVED_LAST; ++type) {
    s.str("");
    s << "form[reserved" << type-StarAttribute::XATTR_FTRESERVED2+2 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }
  std::vector<STOFFVec2i> limits;
  limits.resize(1);
  limits[0]=STOFFVec2i(1000,1016);
  addAttributeItemSet(StarAttribute::XATTR_SET_LINE,"setLine",limits);
  limits[0]=STOFFVec2i(1018,1046);
  addAttributeItemSet(StarAttribute::XATTR_SET_FILL,"setFill",limits);
  limits[0]=STOFFVec2i(1048,1064);
  addAttributeItemSet(StarAttribute::XATTR_SET_TEXT,"setText",limits);

  // ---- sdr ---- svx_svdattr.cxx
  addAttributeBool(StarAttribute::SDRATTR_SHADOW,"shadow", false); // onOff
  addAttributeInt(StarAttribute::SDRATTR_SHADOWXDIST, "shadow[xDist]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_SHADOWYDIST, "shadow[yDist]",4, 0); // metric
  addAttributeUInt(StarAttribute::SDRATTR_SHADOWTRANSPARENCE, "shadow[transparence]",2,0); // percent item
  addAttributeVoid(StarAttribute::SDRATTR_SHADOW3D, "shadow[3d]");
  addAttributeVoid(StarAttribute::SDRATTR_SHADOWPERSP, "shadow[persp]");
  for (int type=StarAttribute::SDRATTR_SHADOWRESERVE1; type<=StarAttribute::SDRATTR_SHADOWRESERVE5; ++type) {
    s.str("");
    s << "shadow[reserved" << type-StarAttribute::SDRATTR_SHADOWRESERVE1+1 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeUInt(StarAttribute::SDRATTR_CAPTIONTYPE, "caption[type]",2,2); // type3
  addAttributeBool(StarAttribute::SDRATTR_CAPTIONFIXEDANGLE,"caption[fixedAngle]", true); // onOff
  addAttributeInt(StarAttribute::SDRATTR_CAPTIONANGLE, "caption[angle]",4, 0); // angle
  addAttributeInt(StarAttribute::SDRATTR_CAPTIONGAP, "caption[gap]",4, 0); // metric
  addAttributeUInt(StarAttribute::SDRATTR_CAPTIONESCDIR, "caption[esc,dir]",2,0); // horizontal
  addAttributeBool(StarAttribute::SDRATTR_CAPTIONESCISREL,"caption[esc,isRel]", true); // yesNo
  addAttributeInt(StarAttribute::SDRATTR_CAPTIONESCREL, "caption[esc,rel]",4, 5000);
  addAttributeInt(StarAttribute::SDRATTR_CAPTIONESCABS, "caption[esc,abs]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_CAPTIONLINELEN, "caption[line,len]",4, 0); // metric
  addAttributeBool(StarAttribute::SDRATTR_CAPTIONFITLINELEN,"caption[fit,lineLen]", true); // yesNo
  for (int type=StarAttribute::SDRATTR_CAPTIONRESERVE1; type<=StarAttribute::SDRATTR_CAPTIONRESERVE5; ++type) {
    s.str("");
    s << "caption[reserved" << type-StarAttribute::SDRATTR_CAPTIONRESERVE1+1 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeInt(StarAttribute::SDRATTR_ECKENRADIUS, "radius[ecken]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TEXT_MINFRAMEHEIGHT, "text[min,frameHeight]",4, 0); // metric
  addAttributeBool(StarAttribute::SDRATTR_TEXT_AUTOGROWHEIGHT,"text[autoGrow,height]", true); // onOff
  addAttributeUInt(StarAttribute::SDRATTR_TEXT_FITTOSIZE, "text[fitToSize]",2,0); // none
  addAttributeInt(StarAttribute::SDRATTR_TEXT_LEFTDIST, "text[left,dist]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TEXT_RIGHTDIST, "text[right,dist]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TEXT_UPPERDIST, "text[upper,dist]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TEXT_LOWERDIST, "text[lower,dist]",4, 0); // metric
  addAttributeUInt(StarAttribute::SDRATTR_TEXT_VERTADJUST, "text[vert,adjust]",2,0); // top
  addAttributeInt(StarAttribute::SDRATTR_TEXT_MAXFRAMEHEIGHT, "text[max,frameHeight]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TEXT_MINFRAMEWIDTH, "text[min,frameWidth]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TEXT_MAXFRAMEWIDTH, "text[max,frameWidth]",4, 0); // metric
  addAttributeBool(StarAttribute::SDRATTR_TEXT_AUTOGROWWIDTH,"text[autoGrow,width]", false); // onOff
  addAttributeUInt(StarAttribute::SDRATTR_TEXT_HORZADJUST, "text[horz,adjust]",2,3); // block
  addAttributeUInt(StarAttribute::SDRATTR_TEXT_ANIKIND, "text[ani,kind]",2,0); // none
  addAttributeUInt(StarAttribute::SDRATTR_TEXT_ANIDIRECTION, "text[ani,direction]",2,0); // left
  addAttributeBool(StarAttribute::SDRATTR_TEXT_ANISTARTINSIDE,"text[ani,startInside]", false); // yesNo
  addAttributeBool(StarAttribute::SDRATTR_TEXT_ANISTOPINSIDE,"text[ani,stopInside]", false); // yesNo
  addAttributeUInt(StarAttribute::SDRATTR_TEXT_ANICOUNT, "text[ani,count]",2,0);
  addAttributeUInt(StarAttribute::SDRATTR_TEXT_ANIDELAY, "text[ani,delay]",2,0);
  addAttributeInt(StarAttribute::SDRATTR_TEXT_ANIAMOUNT, "text[ani,amount]",2, 0);
  addAttributeBool(StarAttribute::SDRATTR_TEXT_CONTOURFRAME,"text[contourFrame]", false); // onOff
  addAttributeVoid(StarAttribute::SDRATTR_XMLATTRIBUTES,"sdr[xmlAttrib]");
  for (int type=StarAttribute::SDRATTR_RESERVE15; type<=StarAttribute::SDRATTR_RESERVE19; ++type) {
    s.str("");
    s << "sdr[reserved" << type-StarAttribute::SDRATTR_RESERVE15+15 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeUInt(StarAttribute::SDRATTR_EDGEKIND, "edge[kind]",2,0); // ortholine
  addAttributeInt(StarAttribute::SDRATTR_EDGENODE1HORZDIST, "edge[node1,hori,dist]",4, 500); // metric
  addAttributeInt(StarAttribute::SDRATTR_EDGENODE1VERTDIST, "edge[node1,vert,dist]",4, 500); // metric
  addAttributeInt(StarAttribute::SDRATTR_EDGENODE2HORZDIST, "edge[node2,hori,dist]",4, 500); // metric
  addAttributeInt(StarAttribute::SDRATTR_EDGENODE2VERTDIST, "edge[node2,vert,dist]",4, 500); // metric
  addAttributeInt(StarAttribute::SDRATTR_EDGENODE1GLUEDIST, "edge[node1,glue,dist]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_EDGENODE2GLUEDIST, "edge[node2,glue,dist]",4, 0); // metric
  addAttributeUInt(StarAttribute::SDRATTR_EDGELINEDELTAANZ, "edge[line,deltaAnz]",2,0);
  addAttributeInt(StarAttribute::SDRATTR_EDGELINE1DELTA, "edge[delta,line1]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_EDGELINE2DELTA, "edge[delta,line2]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_EDGELINE3DELTA, "edge[delta,line3]",4, 0); // metric
  for (int type=StarAttribute::SDRATTR_EDGERESERVE02; type<=StarAttribute::SDRATTR_EDGERESERVE09; ++type) {
    s.str("");
    s << "edge[reserved" << type-StarAttribute::SDRATTR_EDGERESERVE02+2 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeUInt(StarAttribute::SDRATTR_MEASUREKIND, "measure[kind]",2,0); // standard
  addAttributeUInt(StarAttribute::SDRATTR_MEASURETEXTHPOS, "measure[text,hpos]",2,0); // auto
  addAttributeUInt(StarAttribute::SDRATTR_MEASURETEXTVPOS, "measure[text,vpos]",2,0); // auto
  addAttributeInt(StarAttribute::SDRATTR_MEASURELINEDIST, "measure[line,dist]",4, 800); // metric
  addAttributeInt(StarAttribute::SDRATTR_MEASUREHELPLINEOVERHANG, "measure[help,line,overhang]",4, 200); // metric
  addAttributeInt(StarAttribute::SDRATTR_MEASUREHELPLINEDIST, "measure[help,line,dist]",4, 100); // metric
  addAttributeInt(StarAttribute::SDRATTR_MEASUREHELPLINE1LEN, "measure[help,line1,len]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_MEASUREHELPLINE2LEN, "measure[help,line2,len]",4, 0); // metric
  addAttributeBool(StarAttribute::SDRATTR_MEASUREBELOWREFEDGE,"measure[belowRefEdge]", false); // yesNo
  addAttributeBool(StarAttribute::SDRATTR_MEASURETEXTROTA90,"measure[textRot90]", false); // yesNo
  addAttributeBool(StarAttribute::SDRATTR_MEASURETEXTUPSIDEDOWN,"measure[textUpsideDown]", false); // yesNo
  addAttributeInt(StarAttribute::SDRATTR_MEASUREOVERHANG, "measure[overHang]",4, 600); // metric
  addAttributeUInt(StarAttribute::SDRATTR_MEASUREUNIT, "measure[unit]",2,0); // NONE
  addAttributeBool(StarAttribute::SDRATTR_MEASURESHOWUNIT,"measure[showUnit]", false); // yesNo
  addAttributeBool(StarAttribute::SDRATTR_MEASURETEXTAUTOANGLE,"measure[text,isAutoAngle]", true); // yesNo
  addAttributeInt(StarAttribute::SDRATTR_MEASURETEXTAUTOANGLEVIEW,"measure[text,autoAngle]", 4,31500); // angle
  addAttributeBool(StarAttribute::SDRATTR_MEASURETEXTISFIXEDANGLE,"measure[text,isFixedAngle]", false); // yesNo
  addAttributeInt(StarAttribute::SDRATTR_MEASURETEXTFIXEDANGLE,"measure[text,fixedAngle]", 4,0); // angle
  addAttributeInt(StarAttribute::SDRATTR_MEASUREDECIMALPLACES,"measure[decimal,place]", 2,2);
  for (int type=StarAttribute::SDRATTR_MEASURERESERVE05; type<=StarAttribute::SDRATTR_MEASURERESERVE07; ++type) {
    s.str("");
    s << "measure[reserved" << type-StarAttribute::SDRATTR_MEASURERESERVE05+5 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeUInt(StarAttribute::SDRATTR_CIRCKIND, "circle[kind]",2,0); // full
  addAttributeInt(StarAttribute::SDRATTR_CIRCSTARTANGLE, "circle[angle,start]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_CIRCENDANGLE, "circle[angle,end]",4, 36000); // sdrAngle
  for (int type=StarAttribute::SDRATTR_CIRCRESERVE0; type<=StarAttribute::SDRATTR_CIRCRESERVE3; ++type) {
    s.str("");
    s << "circle[reserved" << type-StarAttribute::SDRATTR_CIRCRESERVE0 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeBool(StarAttribute::SDRATTR_OBJMOVEPROTECT,"obj[move,protect]", false); // yesNo
  addAttributeBool(StarAttribute::SDRATTR_OBJSIZEPROTECT,"obj[size,protect]", false); // yesNo
  addAttributeBool(StarAttribute::SDRATTR_OBJPRINTABLE,"obj[printable]", false); // yesNo

  addAttributeUInt(StarAttribute::SDRATTR_LAYERID, "layer[id]",2,0);
  addAttributeInt(StarAttribute::SDRATTR_ALLPOSITIONX, "positionX[all]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ALLPOSITIONY, "positionY[all]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ALLSIZEWIDTH, "size[all,width]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ALLSIZEHEIGHT, "size[all,height]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ONEPOSITIONX, "poitionX[one]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ONEPOSITIONY, "poitionY[one]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ONESIZEWIDTH, "size[one,width]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ONESIZEHEIGHT, "size[one,height]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_LOGICSIZEWIDTH, "size[logic,width]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_LOGICSIZEHEIGHT, "size[logic,height]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ROTATEANGLE, "rotate[angle]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_SHEARANGLE, "shear[angle]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_MOVEX, "moveX",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_MOVEY, "moveY",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_ROTATEONE, "rotate[one]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_HORZSHEARONE, "shear[horzOne]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_VERTSHEARONE, "shear[vertOne]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_ROTATEALL, "rotate[all]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_HORZSHEARALL, "shear[horzAll]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_VERTSHEARALL, "shear[vertAll]",4, 0); // sdrAngle
  addAttributeInt(StarAttribute::SDRATTR_TRANSFORMREF1X, "transform[ref1X]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TRANSFORMREF1Y, "transform[ref1Y]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TRANSFORMREF2X, "transform[ref2X]",4, 0); // metric
  addAttributeInt(StarAttribute::SDRATTR_TRANSFORMREF2Y, "transform[ref2Y]",4, 0); // metric
  addAttributeUInt(StarAttribute::SDRATTR_TEXTDIRECTION, "text[direction]",2,0); // LRTB
  for (int type=StarAttribute::SDRATTR_NOTPERSISTRESERVE2; type<=StarAttribute::SDRATTR_NOTPERSISTRESERVE15; ++type) {
    s.str("");
    s << "notpersist[reserved" << type-StarAttribute::SDRATTR_NOTPERSISTRESERVE2+2 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeInt(StarAttribute::SDRATTR_GRAFRED, "graf[red]",2, 0); // signed percent
  addAttributeInt(StarAttribute::SDRATTR_GRAFGREEN, "graf[green]",2, 0); // signed percent
  addAttributeInt(StarAttribute::SDRATTR_GRAFBLUE, "graf[blue]",2, 0); // signed percent
  addAttributeInt(StarAttribute::SDRATTR_GRAFLUMINANCE, "graf[luminance]",2, 0); // signed percent
  addAttributeInt(StarAttribute::SDRATTR_GRAFCONTRAST, "graf[contrast]",2, 0); // signed percent
  addAttributeUInt(StarAttribute::SDRATTR_GRAFGAMMA, "graf[gamma]",4,100);
  addAttributeUInt(StarAttribute::SDRATTR_GRAFTRANSPARENCE, "graf[transparence]",2,0);
  addAttributeBool(StarAttribute::SDRATTR_GRAFINVERT,"graf[invert]", false); // onOff
  addAttributeUInt(StarAttribute::SDRATTR_GRAFMODE, "graf[mode]",2,0);  // standard
  for (int type=StarAttribute::SDRATTR_GRAFRESERVE3; type<=StarAttribute::SDRATTR_GRAFRESERVE6; ++type) {
    s.str("");
    s << "graf[reserved" << type-StarAttribute::SDRATTR_GRAFRESERVE3+3 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_PERCENT_DIAGONAL, "obj3d[percent,diagonal]",2,10);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_BACKSCALE, "obj3d[backscale]",2,100);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_DEPTH, "obj3d[depth]",4,1000);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_HORZ_SEGS, "obj3d[hori,segments]",4,24);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_VERT_SEGS, "obj3d[vert,segments]",4,24);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_END_ANGLE, "obj3d[endAngle]",4,3600);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_DOUBLE_SIDED,"obj3d[doubleSided]", false);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_NORMALS_KIND, "obj3d[normal,kind]",2,0);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_NORMALS_INVERT,"obj3d[invertNormal]", false);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_X, "obj3d[texture,projX]",2,0);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_Y, "obj3d[texture,projY]",2,0);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_SHADOW_3D,"obj3d[3dShadow]", false);
  addAttributeColor(StarAttribute::SDRATTR_3DOBJ_MAT_COLOR, "obj3d[mat,color]",STOFFColor(0xff,0xb8,0,0)); // check order
  addAttributeColor(StarAttribute::SDRATTR_3DOBJ_MAT_EMISSION, "obj3d[mat,emission]",STOFFColor(0,0,0,0));
  addAttributeColor(StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR, "obj3d[mat,specular]",STOFFColor(0xff,0xff,0xff,0));
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY, "obj3d[matSpecIntensity]",2,15);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_KIND, "obj3d[texture,kind]",2,3);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_MODE, "obj3d[texture,mode]",2,2);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_TEXTURE_FILTER,"obj3d[textureFilter]", false);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_SMOOTH_NORMALS,"obj3d[smoothNormals]", true);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_SMOOTH_LIDS,"obj3d[smoothLids]", false);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_CHARACTER_MODE,"obj3d[charMode]", false);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_CLOSE_FRONT,"obj3d[closeFront]", true);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_CLOSE_BACK,"obj3d[closeBack]", true);
  for (int type=StarAttribute::SDRATTR_3DOBJ_RESERVED_06; type<=StarAttribute::SDRATTR_3DOBJ_RESERVED_20; ++type) {
    s.str("");
    s << "obj3d[reserved" << type-StarAttribute::SDRATTR_3DOBJ_RESERVED_06+6 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_PERSPECTIVE, "scene3d[perspective]",2,1); // perspective
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_DISTANCE, "scene3d[distance]",4,100);
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_FOCAL_LENGTH, "scene3d[focal,length]",4,100);
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_SHADOW_SLANT, "scene3d[shadow,slant]",2,0);
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_SHADE_MODE, "scene3d[shade,mode]",2,2);
  addAttributeBool(StarAttribute::SDRATTR_3DSCENE_TWO_SIDED_LIGHTING, "scene3d[twoSidedLighting]", false);
  for (int type=StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1; type<=StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_8; ++type) {
    s.str("");
    s << "scene3d[light,color" << type-StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1+1 << "]";
    addAttributeColor(StarAttribute::Type(type), s.str(),
                      type==StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1 ? STOFFColor(0xcc,0xcc,0xcc) : STOFFColor(0,0,0,0));
  }
  addAttributeColor(StarAttribute::SDRATTR_3DSCENE_AMBIENTCOLOR, "scene3d[ambient,color]",STOFFColor(0x66,0x66,0x66,0));
  for (int type=StarAttribute::SDRATTR_3DSCENE_LIGHTON_1; type<=StarAttribute::SDRATTR_3DSCENE_LIGHTON_8; ++type) {
    s.str("");
    s << "scene3d[lighton" << type-StarAttribute::SDRATTR_3DSCENE_LIGHTON_1+1 << "]";
    addAttributeBool(StarAttribute::Type(type), s.str(), type==StarAttribute::SDRATTR_3DSCENE_LIGHTON_1);
  }
  for (int type=StarAttribute::SDRATTR_3DSCENE_RESERVED_01; type<=StarAttribute::SDRATTR_3DSCENE_RESERVED_20; ++type) {
    s.str("");
    s << "scene3d[reserved" << type-StarAttribute::SDRATTR_3DSCENE_RESERVED_01+1 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  limits.resize(1);
  limits[0]=STOFFVec2i(1067,1078);
  addAttributeItemSet(StarAttribute::SDRATTR_SET_SHADOW,"setShadow",limits);
  limits[0]=STOFFVec2i(1080,1094);
  addAttributeItemSet(StarAttribute::SDRATTR_SET_CAPTION,"setCaption",limits);
  limits[0]=STOFFVec2i(3989,4037);  // EE_ITEMS_START, EE_ITEMS_END
  addAttributeItemSet(StarAttribute::SDRATTR_SET_OUTLINER,"setOutliner",limits);
  limits[0]=STOFFVec2i(1097,1125);
  addAttributeItemSet(StarAttribute::SDRATTR_SET_MISC,"setMisc",limits);
  limits[0]=STOFFVec2i(1127,1145);
  addAttributeItemSet(StarAttribute::SDRATTR_SET_EDGE,"setEdge",limits);
  limits[0]=STOFFVec2i(1147,1170);
  addAttributeItemSet(StarAttribute::SDRATTR_SET_MEASURE,"setMeasure",limits);
  limits[0]=STOFFVec2i(1172,1178);
  addAttributeItemSet(StarAttribute::SDRATTR_SET_CIRC,"setCircle",limits);
  limits[0]=STOFFVec2i(1180,1242);
  addAttributeItemSet(StarAttribute::SDRATTR_SET_GRAF,"setGraf",limits);
}

}

////////////////////////////////////////////////////////////
// basic attribute function
////////////////////////////////////////////////////////////
void StarAttributeItemSet::addTo(STOFFCellStyle &cell, StarItemPool const *pool) const
{
  StarItemSet finalSet;
  bool newSet=false;
  if (pool && !m_itemSet.m_style.empty()) {
    finalSet=m_itemSet;
    pool->updateUsingStyles(finalSet);
    newSet=true;
  }
  StarItemSet const &set=newSet ? finalSet : m_itemSet;
  for (std::map<int, shared_ptr<StarItem> >::const_iterator it=set.m_whichToItemMap.begin();
       it!=set.m_whichToItemMap.end(); ++it) {
    if (it->second && it->second->m_attribute)
      it->second->m_attribute->addTo(cell, pool);
  }
}

void StarAttributeItemSet::addTo(STOFFFont &font, StarItemPool const *pool) const
{
  StarItemSet finalSet;
  bool newSet=false;
  if (pool && !m_itemSet.m_style.empty()) {
    finalSet=m_itemSet;
    pool->updateUsingStyles(finalSet);
    newSet=true;
  }
  StarItemSet const &set=newSet ? finalSet : m_itemSet;
  for (std::map<int, shared_ptr<StarItem> >::const_iterator it=set.m_whichToItemMap.begin();
       it!=set.m_whichToItemMap.end(); ++it) {
    if (it->second && it->second->m_attribute)
      it->second->m_attribute->addTo(font, pool);
  }
}

void StarAttributeItemSet::addTo(STOFFGraphicStyle &graphic, StarItemPool const *pool) const
{
  StarItemSet finalSet;
  bool newSet=false;
  if (pool && !m_itemSet.m_style.empty()) {
    finalSet=m_itemSet;
    pool->updateUsingStyles(finalSet);
    newSet=true;
  }
  StarItemSet const &set=newSet ? finalSet : m_itemSet;
  for (std::map<int, shared_ptr<StarItem> >::const_iterator it=set.m_whichToItemMap.begin();
       it!=set.m_whichToItemMap.end(); ++it) {
    if (it->second && it->second->m_attribute)
      it->second->m_attribute->addTo(graphic, pool);
  }
}

void StarAttributeItemSet::addTo(STOFFParagraph &para, StarItemPool const *pool) const
{
  StarItemSet finalSet;
  bool newSet=false;
  if (pool && !m_itemSet.m_style.empty()) {
    finalSet=m_itemSet;
    pool->updateUsingStyles(finalSet);
    newSet=true;
  }
  StarItemSet const &set=newSet ? finalSet : m_itemSet;
  for (std::map<int, shared_ptr<StarItem> >::const_iterator it=set.m_whichToItemMap.begin();
       it!=set.m_whichToItemMap.end(); ++it) {
    if (it->second && it->second->m_attribute)
      it->second->m_attribute->addTo(para, pool);
  }
}

void StarAttributeItemSet::print(libstoff::DebugStream &o) const
{
  o << m_debugName;
  if (!m_itemSet.empty()) {
    o << "[";
    for (std::map<int, shared_ptr<StarItem> >::const_iterator it=m_itemSet.m_whichToItemMap.begin();
         it!=m_itemSet.m_whichToItemMap.end(); ++it) {
      if (it->second && it->second->m_attribute)
        it->second->m_attribute->print(o);
      else
        o << "_";
      o << ",";
    }
    o << "]";
  }
  o << ",";
}

bool StarAttributeBool::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  *input>>m_value;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << (m_value ? "true" : "false") << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return pos+1<=endPos;
}

bool StarAttributeColor::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName;
  bool ok=input->readColor(m_value);
  if (!ok) {
    STOFF_DEBUG_MSG(("StarAttributeColor::read: can not read a color\n"));
    f << ",###color,";
  }
  else if (m_value!=m_defValue)
    f << "=" << m_value << ",";
  else
    f << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarAttributeDouble::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  *input>>m_value;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName;
  if (m_value<0 || m_value>0)
    f << "=" << m_value << ",";
  else
    f << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeInt::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  if (m_intSize) m_value=(int) input->readLong(m_intSize);
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeVec2i::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  if (m_intSize) {
    int dim[2];
    for (int i=0; i<2; ++i) dim[i]=(int) input->readLong(m_intSize);
    m_value=STOFFVec2i(dim[0],dim[1]);
  }
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeItemSet::read(StarZone &zone, int /*vers*/, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "StarAttribute[" << zone.getRecordLevel() << "]:" << m_debugName << ",";
  bool ok=object.readItemSet(zone, m_limits, endPos, m_itemSet, object.getCurrentPool(false).get());
  if (!ok) f << "###";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarAttributeUInt::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  if (m_intSize) m_value=(unsigned int) input->readULong(m_intSize);
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeVoid::read(StarZone &zone, int /*vers*/, long /*endPos*/, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << ",";
  ascFile.addPos(input->tell());
  ascFile.addNote(f.str().c_str());
  return true;
}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////

StarAttributeManager::StarAttributeManager() : m_state(new StarAttributeInternal::State)
{
}

StarAttributeManager::~StarAttributeManager()
{
}

shared_ptr<StarAttribute> StarAttributeManager::getDummyAttribute(int id)
{
  if (id<=0)
    return shared_ptr<StarAttribute>(new StarAttributeVoid(StarAttribute::ATTR_CHR_DUMMY1, "unknownAttribute"));
  std::stringstream s;
  s << "attrib" << id;
  return shared_ptr<StarAttribute>(new StarAttributeVoid(StarAttribute::ATTR_CHR_DUMMY1, s.str()));
}

shared_ptr<StarAttribute> StarAttributeManager::getDefaultAttribute(int nWhich)
{
  if (m_state->m_whichToAttributeMap.find(nWhich)!=m_state->m_whichToAttributeMap.end() &&
      m_state->m_whichToAttributeMap.find(nWhich)->second)
    return m_state->m_whichToAttributeMap.find(nWhich)->second->create();
  return getDummyAttribute();
}

shared_ptr<StarAttribute> StarAttributeManager::readAttribute(StarZone &zone, int nWhich, int nVers, long lastPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";

  long pos=input->tell();
  if (m_state->m_whichToAttributeMap.find(nWhich)!=m_state->m_whichToAttributeMap.end() &&
      m_state->m_whichToAttributeMap.find(nWhich)->second) {
    shared_ptr<StarAttribute> attrib=m_state->m_whichToAttributeMap.find(nWhich)->second->create();
    if (!attrib || !attrib->read(zone, nVers, lastPos, object)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read an attribute\n"));
      f << "###bad";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return shared_ptr<StarAttribute>();
    }
    return attrib;
  }

  int val;
  switch (nWhich) {
  case StarAttribute::ATTR_CHR_CHARSETCOLOR: {
    f << "chrAtrCharSetColor=" << input->readULong(1) << ",";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a color\n"));
      f << "###aColor,";
      break;
    }
    if (!color.isBlack())
      f << color << ",";
    break;
  }
  case StarAttribute::ATTR_CHR_ROTATE:
    f << "chrAtrRotate,";
    f << "nVal=" << input->readULong(2) << ",";
    f << "b=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_TWO_LINES: { // checkme
    f << "chrAtrTwoLines=" << input->readULong(1) << ",";
    f << "nStart[unicode]=" << input->readULong(2) << ",";
    f << "nEnd[unicode]=" << input->readULong(2) << ",";
    break;
  }
  case StarAttribute::ATTR_CHR_SCALEW:
  case StarAttribute::ATTR_EE_CHR_SCALEW:
    f << (nWhich==StarAttribute::ATTR_CHR_SCALEW ? "chrAtrScaleW":"eeScaleW") << "=" << input->readULong(2) << ",";
    if (input->tell()<lastPos) {
      f << "nVal=" << input->readULong(2) << ",";
      f << "test=" << input->readULong(2) << ",";
    }
    break;

  // text attribute
  case StarAttribute::ATTR_TXT_INETFMT: {
    f << "textAtrInetFmt,";
    // SwFmtINetFmt::Create
    std::vector<uint32_t> string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find string\n"));
      f << "###url,";
      break;
    }
    if (!string.empty())
      f << "url=" << libstoff::getString(string).cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find string\n"));
      f << "###targert,";
      break;
    }
    if (!string.empty())
      f << "target=" << libstoff::getString(string).cstr() << ",";
    f << "nId1=" << input->readULong(2) << ",";
    f << "nId2=" << input->readULong(2) << ",";
    int nCnt=(int) input->readULong(2);
    bool ok=true;
    f << "libMac=[";
    for (int i=0; i<nCnt; ++i) {
      if (input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a libmac name\n"));
        f << "###libname,";
        ok=false;
        break;
      }
      if (!zone.readString(string)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
        ok=false;
        break;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr() << ":";
      if (!zone.readString(string)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
        ok=false;
        break;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr();
      f << ",";
    }
    f << "],";
    if (!ok) break;
    if (nVers>=1) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find string\n"));
        f << "###aName1,";
        break;
      }
      if (!string.empty())
        f << "aName1=" << libstoff::getString(string).cstr() << ",";
    }
    if (nVers>=2) {
      nCnt=(int) input->readULong(2);
      f << "libMac2=[";
      for (int i=0; i<nCnt; ++i) {
        f << "nCurKey=" << input->readULong(2) << ",";
        if (input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a libmac name\n"));
          f << "###libname2,";
          break;
        }
        if (!zone.readString(string)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
          break;
        }
        else if (!string.empty())
          f << libstoff::getString(string).cstr() << ":";
        if (!zone.readString(string)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
          break;
        }
        else if (!string.empty())
          f << libstoff::getString(string).cstr();
        f << "nScriptType=" << input->readULong(2) << ",";
      }
      f << "],";
    }
    break;
  }
  case StarAttribute::ATTR_TXT_REFMARK: {
    f << "textAtrRefMark,";
    std::vector<uint32_t> string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aName\n"));
      f << "###aName,";
      break;
    }
    if (!string.empty())
      f << "aName=" << libstoff::getString(string).cstr() << ",";
    break;
  }
  case StarAttribute::ATTR_TXT_TOXMARK: {
    f << "textAtrToXMark,";
    int cType=(int) input->readULong(1);
    f << "cType=" << cType << ",";
    f << "nLevel=" << input->readULong(2) << ",";
    std::vector<uint32_t> string;
    int nStringId=0xFFFF;
    if (nVers<1) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aTypeName\n"));
        f << "###aTypeName,";
        break;
      }
      if (!string.empty())
        f << "aTypeName=" << libstoff::getString(string).cstr() << ",";
    }
    else {
      nStringId=(int) input->readULong(2);
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
    }
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aAltText\n"));
      f << "###aAltText,";
      break;
    }
    if (!string.empty())
      f << "aAltText=" << libstoff::getString(string).cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aPrimKey\n"));
      f << "###aPrimKey,";
      break;
    }
    if (!string.empty())
      f << "aPrimKey=" << libstoff::getString(string).cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aSecKey\n"));
      f << "###aSecKey,";
      break;
    }
    if (!string.empty())
      f << "aSecKey=" << libstoff::getString(string).cstr() << ",";
    if (nVers>=2) {
      cType=(int) input->readULong(1);
      f << "cType=" << cType << ",";
      nStringId=(int) input->readULong(2);
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
      f << "cFlags=" << input->readULong(1) << ",";
    }
    if (nVers>=1 && nStringId!=0xFFFF) {
      librevenge::RVNGString poolName;
      if (!zone.getPoolName(nStringId, poolName)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a nId name\n"));
        f << "###nId=" << nStringId << ",";
      }
      else
        f << poolName.cstr() << ",";
    }
    break;
  }
  case StarAttribute::ATTR_TXT_CHARFMT:
    f << "textAtrCharFmt, id=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_TXT_CJK_RUBY: // string("")+bool
    f << "textAtrCJKRuby=" << input->readULong(1) << ",";
    break;

  // field...
  case StarAttribute::ATTR_TXT_FIELD: {
    f << "textAtrField,";
    SWFieldManager fieldManager;
    fieldManager.readField(zone);
    break;
  }
  case StarAttribute::ATTR_TXT_FLYCNT: {
    f << "textAtrFlycnt,";
    if (input->peek()=='o')
      object.getFormatManager()->readSWFormatDef(zone,'o', object);
    else
      object.getFormatManager()->readSWFormatDef(zone,'l', object);
    break;
  }
  case StarAttribute::ATTR_TXT_FTN: {
    f << "textAtrFtn,";
    // sw_sw3npool.cxx SwFmtFtn::Create
    f << "number1=" << input->readULong(2) << ",";
    std::vector<uint32_t> string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the aNumber\n"));
      f << "###aNumber,";
      break;
    }
    if (!string.empty())
      f << "aNumber=" << libstoff::getString(string).cstr() << ",";
    // no sure, find this attribute once with a content here, so ...
    StarObjectText text(object, false); // checkme
    if (!text.readSWContent(zone)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the content\n"));
      f << "###aContent,";
      break;
    }
    if (nVers>=1) {
      uint16_t nSeqNo;
      *input >> nSeqNo;
      if (nSeqNo) f << "nSeqNo=" << nSeqNo << ",";
    }
    if (nVers>=1) {
      uint8_t nFlags;
      *input >> nFlags;
      if (nFlags) f << "nFlags=" << nFlags << ",";
    }
    break;
  }
  case StarAttribute::ATTR_TXT_HARDBLANK: // ok no data
    f << "textAtrHardBlank,";
    if (nVers>=1)
      f << "char=" << char(input->readULong(1)) << ",";
    break;

  // paragraph attribute
  case StarAttribute::ATTR_PARA_LINESPACING:
    f << "parAtrLineSpacing,";
    f << "nPropSpace=" << input->readLong(1) << ",";
    f << "nInterSpace=" << input->readLong(2) << ",";
    f << "nHeight=" << input->readULong(2) << ",";
    f << "nRule=" << input->readULong(1) << ",";
    f << "nInterRule=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_ADJUST:
    f << "parAtrAdjust=" << input->readULong(1) << ",";
    if (nVers>=1) f << "nFlags=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_TABSTOP: {
    f << "parAtrTabStop,";
    int N=(int) input->readULong(1);
    if (input->tell()+7*N>lastPos) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: N is too big\n"));
      f << "###N=" << N << ",";
      N=int(lastPos-input->tell())/7;
    }
    f << "tabs=[";
    for (int i=0; i<N; ++i) {
      int nPos=(int) input->readLong(4);
      f << nPos << "->" << input->readLong(1) << ":" << input->readLong(1) << ":" << input->readLong(1) << ",";
    }
    f << "],";
    break;
  }
  case StarAttribute::ATTR_PARA_HYPHENZONE:
    f << "parAtrHyphenZone=" << input->readLong(1) << ",";
    f << "bHyphenPageEnd=" << input->readLong(1) << ",";
    f << "nMinLead=" << input->readLong(1) << ",";
    f << "nMinTail=" << input->readLong(1) << ",";
    f << "nMaxHyphen=" << input->readLong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_DROP:
    f << "parAtrDrop,";
    f << "nFmt=" << input->readULong(2) << ",";
    f << "nLines1=" << input->readULong(2) << ",";
    f << "nChars1=" << input->readULong(2) << ",";
    f << "nDistance1=" << input->readULong(2) << ",";
    if (nVers>=1)
      f << "bWhole=" << input->readULong(1) << ",";
    else {
      f << "nX=" << input->readULong(2) << ",";
      f << "nY=" << input->readULong(2) << ",";
    }
    break;
  case StarAttribute::ATTR_PARA_NUMRULE: {
    f << "parAtrNumRule,";
    std::vector<uint32_t> string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the sTmp\n"));
      f << "###sTmp,";
      break;
    }
    if (!string.empty())
      f << "sTmp=" << libstoff::getString(string).cstr() << ",";
    if (nVers>0)
      f << "nPoolId=" << input->readULong(2) << ",";
    break;
  }

  // frame parameter
  case StarAttribute::ATTR_FRM_FRM_SIZE:
    f << "frmSize,";
    f << "sizeType=" << input->readULong(1) << ",";
    f << "width=" << input->readULong(4) << ",";
    f << "height=" << input->readULong(4) << ",";
    if (nVers>1)
      f << "percent=" << input->readULong(1) << "x"  << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_LR_SPACE:
  case StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE:
    f << (nWhich==StarAttribute::ATTR_FRM_LR_SPACE ? "lrSpace" : "eeOutLrSpace") << ",";
    f << "left=" << input->readULong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "right=" << input->readULong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "firstLine=" << input->readLong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    if (nVers>=2)
      f << "textLeft=" << input->readLong(2) << ",";
    if (nVers>=3) {
      int8_t autofirst;
      *input >> autofirst;
      if (autofirst) f << "autofirst=" << int(autofirst) << ",";
      long marker=(long) input->readULong(4);
      if (marker==0x599401FE)
        f << "firstLine[bullet]=" << input->readULong(2);
      else
        input->seek(-4, librevenge::RVNG_SEEK_CUR);
    }
    break;
  case StarAttribute::ATTR_FRM_UL_SPACE:
    f << "ulSpace,";
    f << "upper=" << input->readULong(2);
    val=(int) input->readULong(nVers==1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "lower=" << input->readULong(2);
    val=(int) input->readULong(nVers==1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    break;
  case StarAttribute::ATTR_FRM_PAGEDESC:
    f << "pageDesc,";
    if (nVers<1)
      f << "bAutor=" << input->readULong(1) << ",";
    if (nVers< 2)
      f << "nOff=" << input->readULong(2) << ",";
    else {
      unsigned long nOff;
      if (!input->readCompressedULong(nOff)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read nOff\n"));
        f << "###nOff,";
        break;
      }
      f << "nOff=" << nOff << ",";
    }
    f << "nIdx=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_FRM_BREAK:
    f << "pageBreak=" << input->readULong(1) << ",";
    if (nVers<1) input->seek(1, librevenge::RVNG_SEEK_CUR); // dummy
    break;
  case StarAttribute::ATTR_FRM_CNTNT: // checkme
    f << "pageCntnt,";
    while (input->tell()<lastPos) {
      long actPos=input->tell();
      if (input->peek()!='N') {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: find unknown pageCntnt child\n"));
        f << "###child";
        break;
      }
      StarObjectText text(object, false); // checkme
      if (!text.readSWContent(zone) || input->tell()<=actPos) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: find unknown pageCntnt child\n"));
        f << "###child";
        break;
      }
    }
    break;
  case StarAttribute::ATTR_FRM_HEADER:
  case StarAttribute::ATTR_FRM_FOOTER: {
    f << (nWhich==StarAttribute::ATTR_FRM_HEADER ? "header" : "footer") << ",";
    f << "active=" << input->readULong(1) << ",";
    long actPos=input->tell();
    if (actPos==lastPos)
      break;
    object.getFormatManager()->readSWFormatDef(zone,'r',object);
    break;
  }
  case StarAttribute::ATTR_FRM_PROTECT:
    f << "protect,";
    val=(int) input->readULong(1);
    if (val&1) f << "pos[protect],";
    if (val&2) f << "size[protect],";
    if (val&4) f << "cntnt[protect],";
    break;
  case StarAttribute::ATTR_FRM_SURROUND:
    f << "surround=" << input->readULong(1) << ",";
    if (nVers<5) f << "bGold=" << input->readULong(1) << ",";
    if (nVers>1) f << "bAnch=" << input->readULong(1) << ",";
    if (nVers>2) f << "bCont=" << input->readULong(1) << ",";
    if (nVers>3) f << "bOutside1=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_VERT_ORIENT:
  case StarAttribute::ATTR_FRM_HORI_ORIENT:
    f << (nWhich==StarAttribute::ATTR_FRM_VERT_ORIENT ? "vertOrient" : "horiOrient") << ",";
    f << "nPos=" << input->readULong(4) << ",";
    f << "nOrient=" << input->readULong(1) << ",";
    f << "nRel=" << input->readULong(1) << ",";
    if (nWhich==StarAttribute::ATTR_FRM_HORI_ORIENT && nVers>=1) f << "bToggle=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_ANCHOR:
    f << "anchor=" << input->readULong(1) << ",";
    if (nVers<1)
      f << "nId=" << input->readULong(2) << ",";
    else {
      unsigned long nId;
      if (!input->readCompressedULong(nId)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read nId\n"));
        f << "###nId,";
        break;
      }
      else
        f << "nId=" << nId << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_FRMMACRO: { // macitem.cxx SvxMacroTableDtor::Read
    f << "frmMacro,";
    if (nVers>=1) {
      nVers=(uint16_t) input->readULong(2);
      f << "nVersion=" << nVers << ",";
    }
    int16_t nMacros;
    *input>>nMacros;
    f << "macros=[";
    for (int16_t i=0; i<nMacros; ++i) {
      uint16_t nCurKey, eType;
      bool ok=true;
      f << "[";
      *input>>nCurKey;
      if (nCurKey) f << "nCurKey=" << nCurKey << ",";
      for (int j=0; j<2; ++j) {
        std::vector<uint32_t> text;
        if (!zone.readString(text) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a macro string\n"));
          f << "###string" << j << ",";
          ok=false;
          break;
        }
        else if (!text.empty())
          f << (j==0 ? "lib" : "mac") << "=" << libstoff::getString(text).cstr() << ",";
      }
      if (!ok) break;
      if (nVers>=1) {
        *input>>eType;
        if (eType) f << "eType=" << eType << ",";
      }
      f << "],";
    }
    f << "],";
    break;
  }
  case StarAttribute::ATTR_FRM_COL: {
    f << "col,";
    f << "nLineAdj=" << input->readULong(1) << ",";
    f << "bOrtho=" << input->readULong(1) << ",";
    f << "nLineHeight=" << input->readULong(1) << ",";
    f << "nGutterWidth=" << input->readLong(2) << ",";
    int nWishWidth=(int) input->readULong(2);
    f << "nWishWidth=" << nWishWidth << ",";
    f << "nPenStyle=" << input->readULong(1) << ",";
    f << "nPenWidth=" << input->readLong(2) << ",";
    f << "nPenRed=" << (input->readULong(2)>>8) << ",";
    f << "nPenGreen=" << (input->readULong(2)>>8) << ",";
    f << "nPenBlue=" << (input->readULong(2)>>8) << ",";
    int nCol=(int) input->readULong(2);
    f << "N=" << nCol << ",";
    if (nWishWidth==0)
      break;
    if (input->tell()+10*nCol>lastPos) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: nCol is bad\n"));
      f << "###N,";
      break;
    }
    f << "[";
    for (int i=0; i<nCol; ++i) {
      f << "[";
      f << "nWishWidth=" << input->readULong(2) << ",";
      f << "nLeft=" << input->readULong(2) << ",";
      f << "nUpper=" << input->readULong(2) << ",";
      f << "nRight=" << input->readULong(2) << ",";
      f << "nBottom=" << input->readULong(2) << ",";
      f << "],";
    }
    f << "],";
    break;
  }
  case StarAttribute::ATTR_FRM_URL:
    f << "url,";
    if (!StarObjectText::readSWImageMap(zone))
      break;
    if (nVers>=1) {
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the setName\n"));
        f << "###name1,";
        break;
      }
      else if (!text.empty())
        f << "name1=" << libstoff::getString(text).cstr() << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_CHAIN:
    f << "chain,";
    if (nVers>0) {
      f << "prevIdx=" << input->readULong(2) << ",";
      f << "nextIdx=" << input->readULong(2) << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_TEXTGRID: // SwTextGridItem::Create
    f << "textgrid=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_LINENUMBER:
    f << "lineNumber,";
    f << "start=" << input->readULong(4) << ",";
    f << "count[lines]=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_FTN_AT_TXTEND:
  case StarAttribute::ATTR_FRM_END_AT_TXTEND:
    f << (nWhich==StarAttribute::ATTR_FRM_FTN_AT_TXTEND ? "ftnAtTextEnd" : "ednAtTextEnd") << "=" << input->readULong(1) << ",";
    if (nVers>0) {
      f << "offset=" << input->readULong(2) << ",";
      f << "fmtType=" << input->readULong(2) << ",";
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the prefix\n"));
        f << "###prefix,";
        break;
      }
      else if (!text.empty())
        f << "prefix=" << libstoff::getString(text).cstr() << ",";
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the suffix\n"));
        f << "###suffix,";
        break;
      }
      else if (!text.empty())
        f << "suffix=" << libstoff::getString(text).cstr() << ",";
    }
    break;
  // graphic attribute
  case StarAttribute::ATTR_GRF_MIRRORGRF:
    f << "grfMirrorGrf=" << input->readULong(1) << ",";
    if (nVers>0) f << "nToggle=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_GRF_CROPGRF:
    f << "grfCropGrf,";
    f << "top=" << input->readLong(4) << ",";
    f << "left=" << input->readLong(4) << ",";
    f << "right=" << input->readLong(4) << ",";
    f << "bottom=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_BOX_FORMAT:
    f << "boxFormat=" << input->readULong(1) << ",";
    f << "nTmp=" << input->readULong(4) << ",";
    break;
  case StarAttribute::ATTR_BOX_FORMULA: {
    f << "boxFormula,";
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the formula\n"));
      f << "###formula,";
      break;
    }
    else if (!text.empty())
      f << "formula=" << libstoff::getString(text).cstr() << ",";
    break;
  }
  case StarAttribute::ATTR_BOX_VALUE:
    f << "boxAtrValue,";
    if (nVers==0) {
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the dValue\n"));
        f << "###dValue,";
        break;
      }
      else if (!text.empty())
        f << "dValue=" << libstoff::getString(text).cstr() << ",";
    }
    else {
      double res;
      bool isNan;
      if (!input->readDoubleReverted8(res, isNan)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a double\n"));
        f << "##value,";
      }
      else if (res<0||res>0)
        f << "value=" << res << ",";
    }
    break;

  case StarAttribute::ATTR_EE_PARA_NUMBULLET: {
    // svx_numitem.cxx SvxNumRule::SvxNumRule
    f << "eeParaNumBullet,";
    uint16_t version, levelC, nFeatureFlags, nContinuousNumb, numberingType;
    *input >> version >> levelC >> nFeatureFlags >> nContinuousNumb >> numberingType;
    if (version) f << "vers=" << version << ",";
    if (levelC) f << "level[count]=" << levelC << ",";
    if (nFeatureFlags) f << "feature[flags]=" << nFeatureFlags << ",";
    if (nContinuousNumb) f << "continuous[numbering],";
    if (numberingType) f << "number[type]=" << numberingType << ",";
    f << "set=[";
    for (int i=0; i<10; ++i) {
      uint16_t nSet;
      *input>>nSet;
      if (nSet) {
        f << nSet << ",";
        if (!object.getFormatManager()->readNumberFormat(zone, lastPos, object) || input->tell()>lastPos) {
          f << "###";
          break;
        }
      }
      else
        f << "_,";
    }
    f << "],";
    if (version>=2) {
      *input>>nFeatureFlags;
      f << "nFeatureFlags2=" << nFeatureFlags << ",";
    }
    break;
  }
  case StarAttribute::ATTR_EE_PARA_BULLET: {
    // svx_bulitem.cxx SvxBulletItem::SvxBulletItem
    f << "paraBullet,";
    uint16_t style;
    *input >> style;
    if (style==128) {
      StarBitmap bitmap;
      librevenge::RVNGBinaryData data;
      std::string dType;
      if (!bitmap.readBitmap(zone, true, lastPos, data, dType)) {
        f << "###BM,";
        break;
      }
    }
    else {
      // SvxBulletItem::CreateFont
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
        f << "###color,";
        break;
      }
      else if (!col.isBlack())
        f << "color=" << col << ",";
      uint16_t family, encoding, pitch, align, weight, underline, strikeOut, italic;
      *input >> family >> encoding >> pitch >> align >> weight >> underline >> strikeOut >> italic;
      if (family) f << "family=" << family << ",";
      if (encoding) f << "encoding=" << encoding << ",";
      if (pitch) f << "pitch=" << pitch << ",";
      if (weight) f << "weight=" << weight << ",";
      if (underline) f << "underline=" << underline << ",";
      if (strikeOut) f << "strikeOut=" << strikeOut << ",";
      if (italic) f << "italic=" << italic << ",";
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a name\n"));
        f << "###text,";
        break;
      }
      f << libstoff::getString(text).cstr() << ",";
      // if (nVers==1) f << "size=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      bool outline, shadow, transparent;
      *input >> outline >>  shadow >>  transparent;
      if (outline) f << "outline,";
      if (shadow) f << "shadow,";
      if (transparent) f << "transparent,";
    }
    f << "width=" << input->readLong(4) << ",";
    f << "start=" << input->readULong(2) << ",";
    f << "justify=" << input->readULong(1) << ",";
    f << "symbol=" << input->readULong(1) << ",";
    f << "scale=" << input->readULong(2) << ",";
    for (int i=0; i<2; ++i) {
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a name\n"));
        f << "###text,";
        break;
      }
      else if (!text.empty())
        f << (i==0 ? "prev" : "next") << "=" << libstoff::getString(text).cstr() << ",";
    }
    break;
  }
  case StarAttribute::ATTR_EE_FEATURE_FIELD: {
    f << "eeFeatureField,vers=" << nVers << ",";
    // svx_flditem.cxx SvxFieldItem::Create
    if (!object.readPersistData(zone, lastPos)) break;
    return getDummyAttribute(nWhich);
  }

  case StarAttribute::ATTR_SCH_SYMBOL_SIZE:
    f << "symbolSize[sch]=" << input->readLong(4) << "x" << input->readLong(4) << ",";
    break;

  // name or index
  case StarAttribute::XATTR_LINEDASH:
  case StarAttribute::XATTR_LINECOLOR:
  case StarAttribute::XATTR_LINESTART:
  case StarAttribute::XATTR_LINEEND:
  case StarAttribute::XATTR_FILLCOLOR:
  case StarAttribute::XATTR_FILLGRADIENT:
  case StarAttribute::XATTR_FILLHATCH:
  case StarAttribute::XATTR_FILLBITMAP:
  case StarAttribute::XATTR_FILLFLOATTRANSPARENCE:

  case StarAttribute::XATTR_FORMTXTSHDWCOLOR: {
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    if (!text.empty()) f << "name=" << libstoff::getString(text).cstr() << ",";
    int32_t id;
    *input >> id;
    if (id>=0) f << "id=" << id << ",";
    switch (nWhich) {
    case StarAttribute::XATTR_LINEDASH: {
      f << "line[dash],";
      if (id>=0) break;
      int32_t dashStyle;
      uint16_t dots, dashes;
      uint32_t dotLen, dashLen, distance;
      *input >> dashStyle >> dots >> dotLen >> dashes >> dashLen >> distance;
      if (dashStyle) f << "style=" << dashStyle << ",";
      if (dots || dotLen) f << "dots=" << dots << ":" << dotLen << ",";
      if (dashes || dashLen) f << "dashs=" << dashes << ":" << dashLen << ",";
      if (distance) f << "distance=" << distance << ",";
      break;
    }
    case StarAttribute::XATTR_LINECOLOR:
    case StarAttribute::XATTR_FILLCOLOR:
    case StarAttribute::XATTR_FORMTXTSHDWCOLOR: {
      f << (nWhich==StarAttribute::XATTR_LINECOLOR ? "line[color]" :
            nWhich==StarAttribute::XATTR_FILLCOLOR ? "fill[color]" : "shadow[form,color]") << ",";
      if (id>=0) break;
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
        f << "###color,";
        break;
      }
      if (!col.isBlack()) f << col << ",";
      break;
    }
    case StarAttribute::XATTR_LINESTART:
    case StarAttribute::XATTR_LINEEND: {
      f << (nWhich==StarAttribute::XATTR_LINESTART ? "line[start]" : "line[end]") << ",";
      if (id>=0) break;
      uint32_t nPoints;
      *input >> nPoints;
      if (input->tell()+12*long(nPoints)>lastPos) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: bad num point\n"));
        f << "###nPoints=" << nPoints << ",";
        break;
      }
      f << "pts=[";
      for (uint32_t i=0; i<nPoints; ++i)
        f << input->readLong(4) << "x" << input->readLong(4) << ":" << input->readULong(4) << ",";
      f << "],";
      break;
    }
    case StarAttribute::XATTR_FILLGRADIENT:
    case StarAttribute::XATTR_FILLFLOATTRANSPARENCE: {
      f << (nWhich==StarAttribute::XATTR_FILLGRADIENT ? "gradient[fill]" : "transparence[float,fill]")  << ",";
      if (id>=0) break;
      uint16_t type, red, green, blue, red2, green2, blue2;
      *input >> type >> red >> green >> blue >> red2 >> green2 >> blue2;
      if (type) f << "type=" << type << ",";
      f << "startColor=" << STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8)) << ",";
      f << "endColor=" << STOFFColor(uint8_t(red2>>8),uint8_t(green2>>8),uint8_t(blue2>>8)) << ",";
      uint32_t angle;
      uint16_t border, xOffset, yOffset, startIntens, endIntens, nStep;
      *input >> angle >> border >> xOffset >> yOffset >> startIntens >> endIntens;
      if (angle) f << "angle=" << angle << ",";
      if (border) f << "border=" << border << ",";
      f << "offset=" << xOffset << "x" << yOffset << ",";
      f << "intensity=[" << startIntens << "," << endIntens << "],";
      if (nVers>=1) {
        *input >> nStep;
        if (nStep) f << "step=" << nStep << ",";
      }
      if (nWhich==StarAttribute::XATTR_FILLFLOATTRANSPARENCE) {
        bool enabled;
        *input>>enabled;
        if (enabled) f << "enabled,";
      }
      break;
    }
    case StarAttribute::XATTR_FILLHATCH: {
      f << "hatch[fill],";
      if (id>=0) break;
      uint16_t type, red, green, blue;
      int32_t distance, angle;
      *input >> type >> red >> green >> blue >> distance >> angle;
      if (type) f << "type=" << type << ",";
      f << "color=" << STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8)) << ",";
      f << "distance=" << distance << ",";
      f << "angle=" << angle << ",";
      break;
    }
    case StarAttribute::XATTR_FILLBITMAP: {
      f << "bitmap[fill],";
      if (id>=0) break;
      if (nVers==1) {
        int16_t style, type;
        *input >> style >> type;
        if (style) f << "style=" << style << ",";
        if (type) f << "type=" << type << ",";
        if (type==1) {
          if (input->tell()+128+2>lastPos) {
            STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: the zone seems too short\n"));
            f << "###short,";
            break;
          }
          f << "val=[";
          for (int i=0; i<64; ++i) {
            uint16_t value;
            *input>>value;
            if (value) f << std::hex << value << std::dec << ",";
            else f << "_,";
          }
          f << "],";
          for (int i=0; i<2; ++i) {
            STOFFColor col;
            if (!input->readColor(col)) {
              STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
              f << "###color,";
              break;
            }
            f << "col" << i << "=" << col << ",";
          }
          break;
        }
        if (type==2) break;
        if (type!=0) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: unexpected type\n"));
          f << "###type,";
          break;
        }
      }
      StarBitmap bitmap;
      librevenge::RVNGBinaryData data;
      std::string dType;
      if (!bitmap.readBitmap(zone, true, lastPos, data, dType)) break;
      // TODO store the bitmap
      break;
    }

    default:
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: unknown name id\n"));
      f << "###nameId,";
      break;
    }
    break;
  }

  // name or index
  case StarAttribute::SDRATTR_SHADOWCOLOR: {
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    if (!text.empty()) f << "name=" << libstoff::getString(text).cstr() << ",";
    int32_t id;
    *input >> id;
    if (id>=0) f << "id=" << id << ",";
    switch (nWhich) {
    case StarAttribute::SDRATTR_SHADOWCOLOR: {
      f << "sdrShadow[color],";
      if (id>=0) break;
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
        f << "###color,";
        break;
      }
      if (!col.isBlack()) f << col << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: unknown name id\n"));
      f << "###nameId,";
      break;
    }
    break;
  }

  case StarAttribute::SDRATTR_AUTOSHAPE_ADJUSTMENT:
    f << "autoShapeAdjust[sdr],";
    if (nVers) {
      uint32_t n;
      *input >> n;
      if (n) f << "nCount=" << n << ",";
    }
    break;

  case StarAttribute::SDRATTR_MEASURESCALE: //  // sdrFraction
    f << "measure[scale],mult=" << input->readLong(4) << ",div=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_MEASUREFORMATSTRING: { // string
    f << "measure[formatString]=";
    std::vector<uint32_t> format;
    if (!zone.readString(format)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    f << libstoff::getString(format).cstr() << ",";
    break;
  }

  case StarAttribute::SDRATTR_LAYERNAME:
  case StarAttribute::SDRATTR_OBJECTNAME: {
    f << (nWhich==StarAttribute::SDRATTR_LAYERNAME ? "sdrLayerName" : "sdrObjectName") << ",";
    std::vector<uint32_t> name;
    if (!zone.readString(name)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    f << libstoff::getString(name).cstr() << ",";
    break;
  }
  case StarAttribute::SDRATTR_RESIZEXONE: // sdrFraction
  case StarAttribute::SDRATTR_RESIZEYONE:
    f << (nWhich==StarAttribute::SDRATTR_RESIZEXONE ? "sdrResizeX[one]" : "sdrResizeY[one]") << ","
      << "mult=" << input->readLong(4) << ",div=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_RESIZEXALL: // sdrFraction
  case StarAttribute::SDRATTR_RESIZEYALL:
    f << (nWhich==StarAttribute::SDRATTR_RESIZEXALL ? "sdrResizeX[all]" : "sdrResizeY[all]") << ","
      << "mult=" << input->readLong(4) << ",div=" << input->readLong(4) << ",";
    break;

  case StarAttribute::SDRATTR_GRAFCROP: {
    f << "graf[crop],";
    if (nVers==0) break; // clone(0)
    int32_t top, left, right, bottom;
    *input >> top >> left >> right >> bottom;
    f << "crop=" << left << "x" << top << "<->" << right << "x" << bottom << ",";
    break;
  }

  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_1: // SvxVector3DItem (Vector3d)
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_2:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_3:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_4:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_5:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_6:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_7:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_8:
    f << "scene3d[lightDir" << nWhich-StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_1+1 << "=";
    for (int i=0; i<3; ++i) {
      double coord;
      *input >> coord;
      f << coord << (i==2 ? "," : "x");
    }
    break;
  case StarAttribute::ATTR_SPECIAL:
  default:
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: reading not format attribute is not implemented\n"));
    f << "#unimplemented[wh=" << std::hex << nWhich << std::dec << ",";
  }
  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: read too much data\n"));
    f << "###too much,";
    ascFile.addDelimiter(input->tell(), '|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return getDummyAttribute(nWhich); // fixme
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
