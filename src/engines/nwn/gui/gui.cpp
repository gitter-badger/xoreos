/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey, Eclipse and Lycium engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

/** @file engines/nwn/gui/gui.cpp
 *  A NWN GUI.
 */

#include "common/endianness.h"
#include "common/error.h"
#include "common/util.h"

#include "aurora/talkman.h"
#include "aurora/gfffile.h"

#include "engines/aurora/util.h"

#include "engines/nwn/gui/widgets/modelwidget.h"
#include "engines/nwn/gui/widgets/textwidget.h"
#include "engines/nwn/gui/widgets/quadwidget.h"
#include "engines/nwn/gui/widgets/button.h"
#include "engines/nwn/gui/widgets/checkbox.h"
#include "engines/nwn/gui/widgets/close.h"
#include "engines/nwn/gui/widgets/editbox.h"
#include "engines/nwn/gui/widgets/frame.h"
#include "engines/nwn/gui/widgets/label.h"
#include "engines/nwn/gui/widgets/listbox.h"
#include "engines/nwn/gui/widgets/panel.h"
#include "engines/nwn/gui/widgets/scrollbar.h"
#include "engines/nwn/gui/widgets/slider.h"

#include "engines/nwn/gui/gui.h"

namespace Engines {

namespace NWN {

GUI::WidgetContext::WidgetContext(const Aurora::GFFStruct &s, Widget *p) {
	strct = &s;

	widget = 0;
	parent = p;

	type = (WidgetType) strct->getUint("Obj_Type", kWidgetTypeInvalid);
	if (type == kWidgetTypeInvalid)
		throw Common::Exception("Widget without a type");

	tag = strct->getString("Obj_Tag");

	model = strct->getString("Obj_ResRef");

	if (strct->hasField("Obj_Caption")) {
		const Aurora::GFFStruct &caption = strct->getStruct("Obj_Caption");

		font = caption.getString("AurString_Font");

		uint32 strRef = caption.getUint("Obj_StrRef", Aurora::kStrRefInvalid);
		if (strRef != Aurora::kStrRefInvalid)
			text = TalkMan.getString(strRef);
		else
			text = caption.getString("AurString_Text");

		if (text.empty())
			text = " ";
	}
}


GUI::GUI() {
}

GUI::~GUI() {
}

void GUI::load(const Common::UString &resref) {
	_name = resref;

	Aurora::GFFFile *gff = 0;
	try {
		gff = new Aurora::GFFFile(resref, Aurora::kFileTypeGUI, MKTAG('G', 'U', 'I', ' '));

		loadWidget(gff->getTopLevel(), 0);

	} catch (Common::Exception &e) {
		delete gff;

		e.add("Can't load GUI \"%s\"", resref.c_str());
		throw;
	}

	delete gff;
}

void GUI::loadWidget(const Aurora::GFFStruct &strct, Widget *parent) {
	WidgetContext ctx(strct, parent);

	createWidget(ctx);

	addWidget(ctx.widget);

	if (ctx.parent) {
		if (ctx.strct->getString("Obj_Parent") != ctx.parent->getTag())
			throw Common::Exception("Parent's tag != Obj_Parent");

		parent->addChild(*ctx.widget);

		const glm::vec3 ppos = parent->getPosition();
		const glm::vec3 pos = glm::vec3(ctx.strct->getDouble("Obj_X") * 100.0 + ppos.x,
		                                ctx.strct->getDouble("Obj_Y") * 100.0 + ppos.y,
		                                ppos.z - ctx.strct->getDouble("Obj_Z") * 100.0);

		ctx.widget->setPosition(pos);
	} else {
		// We'll ignore these for now, centering the GUI
	}

	initWidget(ctx);

	// Create a caption/label and move the label to its destined position
	WidgetLabel *label = createCaption(ctx);
	if (label && ctx.strct->hasField("Obj_Caption")) {
		const Aurora::GFFStruct &caption = ctx.strct->getStruct("Obj_Caption");

		const glm::vec2 align = glm::vec2(caption.getDouble("AurString_AlignH"),
		                                  caption.getDouble("AurString_AlignV"));
		glm::vec3 position = glm::vec3(ctx.strct->getDouble("Obj_Label_X") * 100.0,
		                               ctx.strct->getDouble("Obj_Label_Y") * 100.0,
		                               ctx.strct->getDouble("Obj_Label_Z") * 100.0);

		if (ctx.type != kWidgetTypeLabel) {
			position += glm::vec3(ctx.widget->getSize() * align, 1.0);
			position -= glm::vec3(label->getSize() / 2.0f, 1.0);
		} else {
			position.y -= label->getSize().y;
			position   -= glm::vec3(label->getSize() * align, 1.0);
		}

		position.z = -position.z;
		label->movePosition(position);
	}

	// uint32 layer = strct.getUint("Obj_Layer");
	// bool locked = strct.getUint("Obj_Locked") != 0;

	// Go down to the children
	if (ctx.strct->hasField("Obj_ChildList")) {
		const Aurora::GFFList &children = ctx.strct->getList("Obj_ChildList");

		for (Aurora::GFFList::const_iterator c = children.begin(); c != children.end(); ++c)
			loadWidget(**c, ctx.widget);
	}
}

void GUI::createWidget(WidgetContext &ctx) {
	// ...BioWare...
	fixWidgetType(ctx.tag, ctx.type);

	if      (ctx.type == kWidgetTypeFrame)
		ctx.widget = new WidgetFrame(*this, ctx.tag, ctx.model);
	else if (ctx.type == kWidgetTypeCloseButton)
		ctx.widget = new WidgetClose(*this, ctx.tag, ctx.model);
	else if (ctx.type == kWidgetTypeCheckBox)
		ctx.widget = new WidgetCheckBox(*this, ctx.tag, ctx.model);
	else if (ctx.type == kWidgetTypePanel)
		ctx.widget = new WidgetPanel(*this, ctx.tag, ctx.model);
	else if (ctx.type == kWidgetTypeLabel)
		ctx.widget = new WidgetLabel(*this, ctx.tag, ctx.font, ctx.text);
	else if (ctx.type == kWidgetTypeSlider)
		ctx.widget = new WidgetSlider(*this, ctx.tag, ctx.model);
	else if (ctx.type == kWidgetTypeEditBox)
		ctx.widget = new WidgetEditBox(*this, ctx.tag, ctx.model, ctx.font);
	else if (ctx.type == kWidgetTypeButton)
		ctx.widget = new WidgetButton(*this, ctx.tag, ctx.model);
	else if (ctx.type == kWidgetTypeListBox)
		ctx.widget = new WidgetListBox(*this, ctx.tag, ctx.model);
	else
		throw Common::Exception("No such widget type %d", ctx.type);

	ModelWidget *widgetModel = dynamic_cast<ModelWidget *>(ctx.widget);
	if (widgetModel)
		initWidget(ctx, *widgetModel);

	TextWidget  *widgetText  = dynamic_cast<TextWidget  *>(ctx.widget);
	if (widgetText)
		initWidget(ctx, *widgetText);
}

void GUI::initWidget(WidgetContext &ctx, ModelWidget &widget) {
}

void GUI::initWidget(WidgetContext &ctx, TextWidget &widget) {
	if (!ctx.strct->hasField("Obj_Caption"))
		return;

	const Aurora::GFFStruct &caption = ctx.strct->getStruct("Obj_Caption");

	float r = caption.getDouble("AurString_ColorR", 1.0);
	float g = caption.getDouble("AurString_ColorG", 1.0);
	float b = caption.getDouble("AurString_ColorB", 1.0);
	float a = caption.getDouble("AurString_ColorA", 1.0);

	widget.setColor(r, g, b, a);
}

void GUI::initWidget(WidgetContext &ctx) {

	initWidget(*ctx.widget);
}

WidgetLabel *GUI::createCaption(WidgetContext &ctx) {
	if (ctx.type == kWidgetTypeLabel)
		return dynamic_cast<WidgetLabel *>(ctx.widget);

	return createCaption(*ctx.strct, ctx.widget);
}

WidgetLabel *GUI::createCaption(const Aurora::GFFStruct &strct, Widget *parent) {
	if (!strct.hasField("Obj_Caption"))
		return 0;

	const Aurora::GFFStruct &caption = strct.getStruct("Obj_Caption");

	Common::UString font = caption.getString("AurString_Font");

	Common::UString text;
	uint32 strRef = caption.getUint("Obj_StrRef", Aurora::kStrRefInvalid);
	if (strRef != Aurora::kStrRefInvalid)
		text = TalkMan.getString(strRef);

	WidgetLabel *label = new WidgetLabel(*this, parent->getTag() + "#Caption", font, text);

	glm::vec3 position = parent->getPosition();
	position.z -= 5.0;
	label->setPosition(position);

	float r = caption.getDouble("AurString_ColorR", 1.0);
	float g = caption.getDouble("AurString_ColorG", 1.0);
	float b = caption.getDouble("AurString_ColorB", 1.0);
	float a = caption.getDouble("AurString_ColorA", 1.0);

	label->setColor(r, g, b, a);

	initWidget(*label);

	parent->addChild(*label);
	addWidget(label);

	return label;
}

void GUI::fixWidgetType(const Common::UString &tag, WidgetType &type) {
}

void GUI::initWidget(Widget &widget) {
}

WidgetFrame *GUI::getFrame(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetFrame *frame = dynamic_cast<WidgetFrame *>(widget);
	if (!frame && vital)
		throw Common::Exception("Vital frame widget \"%s\" doesn't exist", tag.c_str());

	return frame;
}

WidgetClose *GUI::getClose(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetClose *closeButton = dynamic_cast<WidgetClose *>(widget);
	if (!closeButton && vital)
		throw Common::Exception("Vital close button widget \"%s\" doesn't exist", tag.c_str());

	return closeButton;
}

WidgetCheckBox *GUI::getCheckBox(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetCheckBox *checkBox = dynamic_cast<WidgetCheckBox *>(widget);
	if (!checkBox && vital)
		throw Common::Exception("Vital check box widget \"%s\" doesn't exist", tag.c_str());

	return checkBox;
}

WidgetPanel *GUI::getPanel(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetPanel *panel = dynamic_cast<WidgetPanel *>(widget);
	if (!panel && vital)
		throw Common::Exception("Vital panel widget \"%s\" doesn't exist", tag.c_str());

	return panel;
}

WidgetLabel *GUI::getLabel(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetLabel *label = dynamic_cast<WidgetLabel *>(widget);
	if (!label && vital)
		throw Common::Exception("Vital label widget \"%s\" doesn't exist", tag.c_str());

	return label;
}

WidgetSlider *GUI::getSlider(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetSlider *slider = dynamic_cast<WidgetSlider *>(widget);
	if (!slider && vital)
		throw Common::Exception("Vital slider widget \"%s\" doesn't exist", tag.c_str());

	return slider;
}

WidgetEditBox *GUI::getEditBox(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetEditBox *editBox = dynamic_cast<WidgetEditBox *>(widget);
	if (!editBox && vital)
		throw Common::Exception("Vital edit box widget \"%s\" doesn't exist", tag.c_str());

	return editBox;
}

WidgetButton *GUI::getButton(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetButton *button = dynamic_cast<WidgetButton *>(widget);
	if (!button && vital)
		throw Common::Exception("Vital button widget \"%s\" doesn't exist", tag.c_str());

	return button;
}

WidgetListBox *GUI::getListBox(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetListBox *listBox = dynamic_cast<WidgetListBox *>(widget);
	if (!listBox && vital)
		throw Common::Exception("Vital listBox widget \"%s\" doesn't exist", tag.c_str());

	return listBox;
}

WidgetScrollbar *GUI::getScrollbar(const Common::UString &tag, bool vital) {
	Widget *widget = getWidget(tag, vital);
	if (!widget)
		return 0;

	WidgetScrollbar *scrollbar = dynamic_cast<WidgetScrollbar *>(widget);
	if (!scrollbar && vital)
		throw Common::Exception("Vital scrollbar widget \"%s\" doesn't exist", tag.c_str());

	return scrollbar;
}

} // End of namespace NWN

} // End of namespace Engines
