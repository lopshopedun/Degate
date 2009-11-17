/*                                                                              
                                                                                
This file is part of the IC reverse engineering tool degate.                    
                                                                                
Copyright 2008, 2009 by Martin Schobert                                         
                                                                                
Degate is free software: you can redistribute it and/or modify                  
it under the terms of the GNU General Public License as published by            
the Free Software Foundation, either version 3 of the License, or               
any later version.                                                              
                                                                                
Degate is distributed in the hope that it will be useful,                       
but WITHOUT ANY WARRANTY; without even the implied warranty of                  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   
GNU General Public License for more details.                                    
                                                                                
You should have received a copy of the GNU General Public License               
along with degate. If not, see <http://www.gnu.org/licenses/>.                  
                                                                                
*/

#include "GateConfigWin.h"
#include "GladeFileLoader.h"

#include <gdkmm/window.h>
#include <gtkmm/stock.h>
#include <libglademm.h>

#include <stdlib.h>
#include <iostream>

#include "GateTemplate.h"
#include <boost/format.hpp>

using namespace degate;


GateConfigWin::GateConfigWin(Gtk::Window *parent, 
			     LogicModel_shptr lmodel,
			     GateTemplate_shptr gate_template) :
  GladeFileLoader("gate_create.glade", "gate_create_dialog") {

  this->lmodel = lmodel;
  this->gate_template = gate_template;

  this->parent = parent;

  if(pDialog) {
    //Get the Glade-instantiated Button, and connect a signal handler:
    Gtk::Button* pButton = NULL;
    
    // connect signals
    refXml->get_widget("cancel_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_cancel_button_clicked));
    
    refXml->get_widget("ok_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_ok_button_clicked) );

    
    refXml->get_widget("port_add_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_port_add_button_clicked) );
    
    refXml->get_widget("port_remove_button", pButton);
    if(pButton)
      pButton->signal_clicked().connect(sigc::mem_fun(*this, &GateConfigWin::on_port_remove_button_clicked) );
    
   
      refListStore_ports = Gtk::ListStore::create(m_Columns);
      
      refXml->get_widget("treeview_ports", pTreeView_ports);
      if(pTreeView_ports) {
	pTreeView_ports->set_model(refListStore_ports);
	pTreeView_ports->append_column("Port ID", m_Columns.m_col_id);
	pTreeView_ports->append_column_editable("Port Name", m_Columns.m_col_text);
	pTreeView_ports->append_column_editable("In", m_Columns.m_col_inport);
	pTreeView_ports->append_column_editable("Out", m_Columns.m_col_outport);
      }
      

      color_t frame_color = gate_template->get_frame_color();
      color_t fill_color = gate_template->get_fill_color();

      refXml->get_widget("colorbutton_fill_color", colorbutton_fill_color);
      if(colorbutton_fill_color != NULL) {
	Gdk::Color c;
	if(fill_color != 0) {
	  c.set_red(MASK_R(fill_color) << 8);
	  c.set_green(MASK_G(fill_color) << 8);
	  c.set_blue(MASK_B(fill_color) << 8);
	  colorbutton_fill_color->set_alpha(MASK_A(fill_color) << 8);
	  colorbutton_fill_color->set_color(c);
	}
	else {
	  c.set_red(0x30 << 8);
	  c.set_green(0x30 << 8);
	  c.set_blue(0x30 << 8);
	  colorbutton_fill_color->set_alpha(0xa0 << 8);
	  colorbutton_fill_color->set_color(c);
	}
      }

      refXml->get_widget("colorbutton_frame_color", colorbutton_frame_color);
      if(colorbutton_frame_color != NULL) {
	Gdk::Color c;
	if(frame_color != 0) {
	  c.set_red(MASK_R(frame_color) << 8);
	  c.set_green(MASK_G(frame_color) << 8);
	  c.set_blue(MASK_B(frame_color) << 8);
	  colorbutton_frame_color->set_alpha(MASK_A(frame_color) << 8);
	  colorbutton_frame_color->set_color(c);
	}
	else {
	  c.set_red(0xa0 << 8);
	  c.set_green(0xa0 << 8);
	  c.set_blue(0xa0 << 8);
	  colorbutton_fill_color->set_alpha(0x7f << 8);
	  colorbutton_fill_color->set_color(c);
	}
      }


      for(GateTemplate::port_iterator iter = gate_template->ports_begin();
	  iter != gate_template->ports_end();
	  ++iter) {
		
	Gtk::TreeModel::Row row = *(refListStore_ports->append());

	GateTemplatePort_shptr tmpl_port = *iter;

	debug(TM, "PORT NAME: [%s]", tmpl_port->get_name().c_str());

	row[m_Columns.m_col_inport] = tmpl_port->is_inport();
	row[m_Columns.m_col_outport] = tmpl_port->is_outport();
	row[m_Columns.m_col_text] = tmpl_port->get_name();
	row[m_Columns.m_col_id] = tmpl_port->get_object_id();

	original_ports.push_back(tmpl_port);
      }
      
      refXml->get_widget("entry_short_name", entry_short_name);
      refXml->get_widget("entry_description", entry_description);

      if(entry_short_name) 
	entry_short_name->set_text(gate_template->get_name());
      if(entry_description) 
	entry_description->set_text(gate_template->get_description());

    }
}

GateConfigWin::~GateConfigWin() {
}


bool GateConfigWin::run() {
  pDialog->run();
  return result;
}

void GateConfigWin::on_ok_button_clicked() {
  Glib::ustring name_str;
  object_id_t id;
  GateTemplatePort::PORT_TYPE port_type;

  // get text content and set it to the gate template
  gate_template->set_name(entry_short_name->get_text().c_str());
  gate_template->set_description(entry_description->get_text().c_str());

  // get ports
  typedef Gtk::TreeModel::Children type_children;

  type_children children = refListStore_ports->children();
  for(type_children::iterator iter = children.begin(); iter != children.end(); ++iter) {
    Gtk::TreeModel::Row row = *iter;
    name_str = row[m_Columns.m_col_text];
    id = row[m_Columns.m_col_id];
    
    if(row[m_Columns.m_col_inport] == true &&
       row[m_Columns.m_col_outport] == true) port_type = GateTemplatePort::PORT_TYPE_TRISTATE;
    else if(row[m_Columns.m_col_inport] == true) port_type = GateTemplatePort::PORT_TYPE_IN;
    else if(row[m_Columns.m_col_outport] == true) port_type = GateTemplatePort::PORT_TYPE_OUT;
    else port_type = GateTemplatePort::PORT_TYPE_UNDEFINED;

    if(id == 0) {
      // create a new template port
      GateTemplatePort_shptr new_template_port(new GateTemplatePort(port_type));

      new_template_port->set_object_id(lmodel->get_new_object_id());
      new_template_port->set_name(name_str);

      lmodel->add_template_port_to_gate_template(gate_template, new_template_port);
    }
    else {
      GateTemplatePort_shptr tmpl_port = gate_template->get_template_port(id);
      tmpl_port->set_name(name_str);
      tmpl_port->set_port_type(port_type);
      original_ports.remove(tmpl_port);      
    }


  }


  // remaining entries in original_ports are not in list store. we can
  // remove them from the logic model
  std::list<GateTemplatePort_shptr>::iterator i;
  for(i = original_ports.begin(); i != original_ports.end(); ++i) {
    GateTemplatePort_shptr tmpl_port = *i;
    debug(TM, "remove port from templates / gates with id=%d", 
	  tmpl_port->get_object_id());

    lmodel->remove_template_port_from_gate_template(gate_template, tmpl_port);
  }


  Gdk::Color fill_color = colorbutton_fill_color->get_color();
  Gdk::Color frame_color = colorbutton_frame_color->get_color();

  gate_template->set_fill_color(MERGE_CHANNELS(fill_color.get_red() >> 8,
					       fill_color.get_green() >> 8,
					       fill_color.get_blue() >> 8,
					       colorbutton_fill_color->get_alpha() >> 8));
  gate_template->set_frame_color(MERGE_CHANNELS(frame_color.get_red() >> 8,
						frame_color.get_green() >> 8,
						frame_color.get_blue() >> 8,
						colorbutton_frame_color->get_alpha() >> 8));

  pDialog->hide();
  result = true;
}

void GateConfigWin::on_cancel_button_clicked() {
  pDialog->hide();
  result = false;
}

void GateConfigWin::on_port_add_button_clicked() {

  
  Gtk::TreeNodeChildren::size_type children_size = refListStore_ports->children().size();

  Gtk::TreeModel::Row row = *(refListStore_ports->append()); 
  row[m_Columns.m_col_id] = 0;

  if(children_size == 0) {
    row[m_Columns.m_col_text] = "y";
    row[m_Columns.m_col_outport] = true;
  }
  else {
    if(children_size - 1 < 'q' - 'a') {
      unsigned char symbol = 'a' + children_size - 1;
      boost::format f("%1%");
      f % symbol;
      row[m_Columns.m_col_text] = f.str();
    }
    else
      row[m_Columns.m_col_text] = "click to edit";

    row[m_Columns.m_col_inport] = true;
  }

}

void GateConfigWin::on_port_remove_button_clicked() {
  Glib::RefPtr<Gtk::TreeSelection> refTreeSelection =  pTreeView_ports->get_selection();
  if(refTreeSelection) {
    Gtk::TreeModel::iterator iter = refTreeSelection->get_selected();
    if(*iter) refListStore_ports->erase(iter);
  }
}
