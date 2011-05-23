//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: functions.cpp,v 1.20.2.19 2011/05/05 20:10 flo93 Exp $
//  (C) Copyright 2011 Florian Jung (flo93@sourceforge.net)
//=========================================================

#include "functions.h"
#include "song.h"

#include "event.h"
#include "audio.h"
#include "gconfig.h"

#include <iostream>

#include <QMessageBox>

using namespace std;

GateTime* gatetime_dialog=NULL;
Velocity* velocity_dialog=NULL;
Quantize* quantize_dialog=NULL;
Remove* erase_dialog=NULL;
DelOverlaps* del_overlaps_dialog=NULL;
Setlen* set_notelen_dialog=NULL;
Move* move_notes_dialog=NULL;
Transpose* transpose_dialog=NULL;
Crescendo* crescendo_dialog=NULL;

void init_function_dialogs(QWidget* parent)
{
	gatetime_dialog = new GateTime(parent);
	velocity_dialog = new Velocity(parent);
	quantize_dialog = new Quantize(parent);
	erase_dialog = new Remove(parent);
	del_overlaps_dialog = new DelOverlaps(parent);
	set_notelen_dialog = new Setlen(parent);
	move_notes_dialog = new Move(parent);
	transpose_dialog = new Transpose(parent);
	crescendo_dialog = new Crescendo(parent);
}

set<Part*> partlist_to_set(PartList* pl)
{
	set<Part*> result;
	
	for (PartList::iterator it=pl->begin(); it!=pl->end(); it++)
		result.insert(it->second);
	
	return result;
}

bool is_relevant(const Event& event, const Part* part, int range)
{
	unsigned tick;
	
	if (event.type()!=Note) return false;
	
	switch (range)
	{
		case 0: return true;
		case 1: return event.selected();
		case 2: tick=event.tick()+part->tick(); return (tick >= song->lpos()) && (tick < song->rpos());
		case 3: return is_relevant(event,part,1) && is_relevant(event,part,2);
		default: cout << "ERROR: ILLEGAL FUNCTION CALL in is_relevant: range is illegal: "<<range<<endl;
		         return false;
	}
}


map<Event*, Part*> get_events(const set<Part*>& parts, int range)
{
	map<Event*, Part*> events;
	
	for (set<Part*>::iterator part=parts.begin(); part!=parts.end(); part++)
		for (iEvent event=(*part)->events()->begin(); event!=(*part)->events()->end(); event++)
			if (is_relevant(event->second, *part, range))
				events.insert(pair<Event*, Part*>(&event->second, *part));
	
	return events;
}


bool modify_notelen(const set<Part*>& parts)
{
	if (!gatetime_dialog->exec())
		return false;
		
	modify_notelen(parts,gatetime_dialog->range,gatetime_dialog->rateVal,gatetime_dialog->offsetVal);
	
	return true;
}

bool modify_velocity(const set<Part*>& parts)
{
	if (!velocity_dialog->exec())
		return false;
		
	modify_velocity(parts,velocity_dialog->range,velocity_dialog->rateVal,velocity_dialog->offsetVal);
	
	return true;
}

bool quantize_notes(const set<Part*>& parts)
{
	if (!quantize_dialog->exec())
		return false;
		
	quantize_notes(parts, quantize_dialog->range, (config.division*4)/(1<<quantize_dialog->raster_power2),
	               quantize_dialog->quant_len, quantize_dialog->strength, quantize_dialog->swing,
	               quantize_dialog->threshold);
	
	return true;
}

bool erase_notes(const set<Part*>& parts)
{
	if (!erase_dialog->exec())
		return false;
		
	erase_notes(parts,erase_dialog->range, erase_dialog->velo_threshold, erase_dialog->velo_thres_used, 
	                                       erase_dialog->len_threshold, erase_dialog->len_thres_used );
	
	return true;
}

bool delete_overlaps(const set<Part*>& parts)
{
	if (!del_overlaps_dialog->exec())
		return false;
		
	delete_overlaps(parts,erase_dialog->range);
	
	return true;
}

bool set_notelen(const set<Part*>& parts)
{
	if (!set_notelen_dialog->exec())
		return false;
		
	set_notelen(parts,set_notelen_dialog->range,set_notelen_dialog->len);
	
	return true;
}

bool move_notes(const set<Part*>& parts)
{
	if (!move_notes_dialog->exec())
		return false;
		
	move_notes(parts,move_notes_dialog->range,move_notes_dialog->amount);
	
	return true;
}

bool transpose_notes(const set<Part*>& parts)
{
	if (!transpose_dialog->exec())
		return false;
		
	transpose_notes(parts,transpose_dialog->range,transpose_dialog->amount);
	
	return true;
}

bool crescendo(const set<Part*>& parts)
{
	if (song->rpos() <= song->lpos())
	{
		QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Please first select the range for crescendo with the loop markers."));
		return false;
	}
	
	if (!crescendo_dialog->exec())
		return false;
		
	crescendo(parts,crescendo_dialog->range,crescendo_dialog->start_val,crescendo_dialog->end_val,crescendo_dialog->absolute);
	
	return true;
}



void modify_velocity(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events = get_events(parts, range);
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		song->startUndo();
		
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
			
			int velo = event.velo();

			velo = (velo * rate) / 100;
			velo += offset;

			if (velo <= 0)
				velo = 1;
			else if (velo > 127)
				velo = 127;
				
			if (event.velo() != velo)
			{
				Event newEvent = event.clone();
				newEvent.setVelo(velo);
				// Indicate no undo, and do not do port controller values and clone parts. 
				audio->msgChangeEvent(event, newEvent, part, false, false, false);
			}
		}
		
		song->endUndo(SC_EVENT_MODIFIED);
	}
}

void modify_off_velocity(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events = get_events(parts, range);
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		song->startUndo();
		
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
			
			int velo = event.veloOff();

			velo = (velo * rate) / 100;
			velo += offset;

			if (velo <= 0)
				velo = 1;
			else if (velo > 127)
				velo = 127;
				
			if (event.veloOff() != velo)
			{
				Event newEvent = event.clone();
				newEvent.setVeloOff(velo);
				// Indicate no undo, and do not do port controller values and clone parts. 
				audio->msgChangeEvent(event, newEvent, part, false, false, false);
			}
		}
		
		song->endUndo(SC_EVENT_MODIFIED);
	}
}

void modify_notelen(const set<Part*>& parts, int range, int rate, int offset)
{
	map<Event*, Part*> events = get_events(parts, range);
	
	if ( (!events.empty()) && ((rate!=100) || (offset!=0)) )
	{
		song->startUndo();
		
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

			unsigned int len = event.lenTick(); //prevent compiler warning: comparison singed/unsigned

			len = (len * rate) / 100;
			len += offset;

			if (len <= 0)
				len = 1;
				
			if (event.lenTick() != len)
			{
				Event newEvent = event.clone();
				newEvent.setLenTick(len);
				// Indicate no undo, and do not do port controller values and clone parts. 
				audio->msgChangeEvent(event, newEvent, part, false, false, false);
			}
		}
		
		song->endUndo(SC_EVENT_MODIFIED);
	}
}

void set_notelen(const set<Part*>& parts, int range, int len)
{
	modify_notelen(parts, range, 0, len);
}

unsigned quantize_tick(unsigned tick, unsigned raster, int swing)
{
	//find out the nearest tick and the distance to it:
	//this is so complicated because this function supports
	//swing: if swing is 50, the resulting rhythm is not
	//"daa daa daa daa" but "daaaa da daaaa da"...
	int tick_dest1 = AL::sigmap.raster1(tick, raster*2); //round down
	int tick_dest2 = tick_dest1 + raster + raster*swing/100;
	int tick_dest3 = tick_dest1 + raster*2;

	int tick_diff1 = tick_dest1 - tick;
	int tick_diff2 = tick_dest2 - tick;
	int tick_diff3 = tick_dest3 - tick;
	
	if ((abs(tick_diff1) <= abs(tick_diff2)) && (abs(tick_diff1) <= abs(tick_diff3))) //tick_dest1 is the nearest tick
		return tick_dest1;
	else if ((abs(tick_diff2) <= abs(tick_diff1)) && (abs(tick_diff2) <= abs(tick_diff3))) //tick_dest2 is the nearest tick
		return tick_dest2;
	else
		return tick_dest3;
}

void quantize_notes(const set<Part*>& parts, int range, int raster, bool quant_len, int strength, int swing, int threshold)
{
	map<Event*, Part*> events = get_events(parts, range);
	bool undo_started=false;
	
	if (!events.empty())
	{
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

			unsigned begin_tick = event.tick() + part->tick();
			int begin_diff = quantize_tick(begin_tick, raster, swing) - begin_tick;

			if (abs(begin_diff) > threshold)
				begin_tick = begin_tick + begin_diff*strength/100;


			unsigned len=event.lenTick();
			
			unsigned end_tick = begin_tick + len;
			int len_diff = quantize_tick(end_tick, raster, swing) - end_tick;
				
			if ((abs(len_diff) > threshold) && quant_len)
				len = len + len_diff*strength/100;

			if (len <= 0)
				len = 1;

				
			if ( (event.lenTick() != len) || (event.tick() + part->tick() != begin_tick) )
			{
				if (!undo_started)
				{
					song->startUndo();
					undo_started=true;
				}
				
				Event newEvent = event.clone();
				newEvent.setTick(begin_tick - part->tick());
				newEvent.setLenTick(len);
				// Indicate no undo, and do not do port controller values and clone parts. 
				audio->msgChangeEvent(event, newEvent, part, false, false, false);
			}
		}
		
		if (undo_started) song->endUndo(SC_EVENT_MODIFIED);
	}
}

void erase_notes(const set<Part*>& parts, int range, int velo_threshold, bool velo_thres_used, int len_threshold, bool len_thres_used)
{
	map<Event*, Part*> events = get_events(parts, range);
	
	if (!events.empty())
	{
		song->startUndo();
		
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
			if ( (!velo_thres_used && !len_thres_used) ||
			     (velo_thres_used && event.velo() < velo_threshold) ||
			     (len_thres_used && int(event.lenTick()) < len_threshold) )
				audio->msgDeleteEvent(event, part, false, false, false);
		}
		
		song->endUndo(SC_EVENT_REMOVED);
	}
}

void transpose_notes(const set<Part*>& parts, int range, signed int halftonesteps)
{
	map<Event*, Part*> events = get_events(parts, range);
	
	if ( (!events.empty()) && (halftonesteps!=0) )
	{
		song->startUndo();
		
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

			Event newEvent = event.clone();
			int pitch = event.pitch()+halftonesteps;
			if (pitch > 127) pitch=127;
			if (pitch < 0) pitch=0;
			newEvent.setPitch(pitch);
			// Indicate no undo, and do not do port controller values and clone parts. 
			audio->msgChangeEvent(event, newEvent, part, false, false, false);
		}
		
		song->endUndo(SC_EVENT_MODIFIED);
	}
}

void crescendo(const set<Part*>& parts, int range, int start_val, int end_val, bool absolute)
{
	map<Event*, Part*> events = get_events(parts, range);
	
	int from=song->lpos();
	int to=song->rpos();
	
	if ( (!events.empty()) && (to>from) )
	{
		song->startUndo();
		
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;
			
			unsigned tick = event.tick() + part->tick();
			float curr_val= (float)start_val  +  (float)(end_val-start_val) * (tick-from) / (to-from);
			
			Event newEvent = event.clone();
			int velo = event.velo();

			if (absolute)
				velo=curr_val;
			else
				velo=curr_val*velo/100;

			if (velo > 127) velo=127;
			if (velo <= 0) velo=1;
			newEvent.setVelo(velo);
			// Indicate no undo, and do not do port controller values and clone parts. 
			audio->msgChangeEvent(event, newEvent, part, false, false, false);
		}
		
		song->endUndo(SC_EVENT_MODIFIED);
	}
}

void move_notes(const set<Part*>& parts, int range, signed int ticks) //TODO FINDMICH: safety checks
{
	map<Event*, Part*> events = get_events(parts, range);
	
	if ( (!events.empty()) && (ticks!=0) )
	{
		song->startUndo();
		
		for (map<Event*, Part*>::iterator it=events.begin(); it!=events.end(); it++)
		{
			Event& event=*(it->first);
			Part* part=it->second;

			Event newEvent = event.clone();
			newEvent.setTick(event.tick()+ticks);
			// Indicate no undo, and do not do port controller values and clone parts. 
			audio->msgChangeEvent(event, newEvent, part, false, false, false);
		}
		
		song->endUndo(SC_EVENT_MODIFIED);
	}
}

void delete_overlaps(const set<Part*>& parts, int range)
{
	map<Event*, Part*> events = get_events(parts, range);
	bool undo_started=false;
	
	set<Event*> deleted_events;
	
	if (!events.empty())
	{
		for (map<Event*, Part*>::iterator it1=events.begin(); it1!=events.end(); it1++)
		{
			Event& event1=*(it1->first);
			Part* part1=it1->second;
			
			// we may NOT optimize by letting it2 start at (it1 +1); this optimisation
			// is only allowed when events was sorted by time. it is, however, sorted
			// randomly by pointer.
			for (map<Event*, Part*>::iterator it2=events.begin(); it2!=events.end(); it2++)
			{
				Event& event2=*(it2->first);
				Part* part2=it2->second;
				
				if ( (part1->events()==part2->events()) && // part1 and part2 are the same or are duplicates
				     (&event1 != &event2) &&               // and event1 and event2 aren't the same
				     (deleted_events.find(&event2) == deleted_events.end()) ) //and event2 hasn't been deleted before
				{
					if ( (event1.pitch() == event2.pitch()) &&
					     (event1.tick() <= event2.tick()) &&
						   (event1.endTick() > event2.tick()) ) //they overlap
					{
						if (undo_started==false)
						{
							song->startUndo();
							undo_started=true;
						}
						
						int new_len = event2.tick() - event1.tick();

						if (new_len==0)
						{
							audio->msgDeleteEvent(event1, part1, false, false, false);
							deleted_events.insert(&event1);
						}
						else
						{
							Event new_event1 = event1.clone();
							new_event1.setLenTick(new_len);
							
							audio->msgChangeEvent(event1, new_event1, part1, false, false, false);
						}
					}
				}
			}
		}
		
		if (undo_started) song->endUndo(SC_EVENT_MODIFIED);
	}
}



void read_function_dialog_config(Xml& xml)
{
	if (erase_dialog==NULL)
	{
		cout << "ERROR: THIS SHOULD NEVER HAPPEN: read_function_dialog_config() called, but\n"
		        "                                 dialogs are still uninitalized (NULL)!"<<endl;
		return;
	}
		
	for (;;)
	{
		Xml::Token token = xml.parse();
		if (token == Xml::Error || token == Xml::End)
			break;
			
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::TagStart:
				if (tag == "mod_len")
					gatetime_dialog->read_configuration(xml);
				else if (tag == "mod_velo")
					velocity_dialog->read_configuration(xml);
				else if (tag == "quantize")
					quantize_dialog->read_configuration(xml);
				else if (tag == "erase")
					erase_dialog->read_configuration(xml);
				else if (tag == "del_overlaps")
					del_overlaps_dialog->read_configuration(xml);
				else if (tag == "setlen")
					set_notelen_dialog->read_configuration(xml);
				else if (tag == "move")
					move_notes_dialog->read_configuration(xml);
				else if (tag == "transpose")
					transpose_dialog->read_configuration(xml);
				else if (tag == "crescendo")
					crescendo_dialog->read_configuration(xml);
				else
					xml.unknown("function_dialogs");
				break;
				
			case Xml::TagEnd:
				if (tag == "dialogs")
					return;
				
			default:
				break;
		}
	}
}

void write_function_dialog_config(int level, Xml& xml)
{
	xml.tag(level++, "dialogs");

	gatetime_dialog->write_configuration(level, xml);
	velocity_dialog->write_configuration(level, xml);
	quantize_dialog->write_configuration(level, xml);
	erase_dialog->write_configuration(level, xml);
	del_overlaps_dialog->write_configuration(level, xml);
	set_notelen_dialog->write_configuration(level, xml);
	move_notes_dialog->write_configuration(level, xml);
	transpose_dialog->write_configuration(level, xml);
	crescendo_dialog->write_configuration(level, xml);

	xml.tag(level, "/dialogs");
}