//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tempo.h,v 1.2.2.1 2006/09/19 19:07:09 spamatica Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================
#ifndef KEYEVENT_H
#define KEYEVENT_H

#include <map>

#ifndef MAX_TICK
#define MAX_TICK (0x7fffffff/100)
#endif

class Xml;

//don't change this enum! changing the numeric values will affect
//all files using key_enum, and even worse:
//PREVIOUSLY SAVED FILES WILL BE CORRUPT because the keys are
//stored as integers. when the integer -> key mapping changes
//(by inserting or removing elements, for example), this will
//break stuff! (flo)
enum key_enum
{
	KEY_SHARP_BEGIN,
	KEY_C,   // C or am, uses # for "black keys"
	KEY_G,
	KEY_D,
	KEY_A,
	KEY_E,
	KEY_B, // or H in german.
	KEY_FIS, //replaces F with E#
	KEY_SHARP_END,
	KEY_B_BEGIN,
	KEY_C_B,  // the same as C, but uses b for "black keys"
	KEY_F,
	KEY_BES, // or B in german
	KEY_ES,
	KEY_AS,
	KEY_DES,
	KEY_GES, //sounds like FIS, but uses b instead of #
	KEY_B_END
};




//---------------------------------------------------------
//   Key Event
//---------------------------------------------------------

struct KeyEvent {
      key_enum key;
      unsigned tick;

      int read(Xml&);
      void write(int, Xml&, int) const;

      KeyEvent() { }
      KeyEvent(key_enum k, unsigned tk) {
            key = k;
            tick  = tk;
            }
      };

//---------------------------------------------------------
//   KeyList
//---------------------------------------------------------

typedef std::map<unsigned, KeyEvent, std::less<unsigned> > KEYLIST;
typedef KEYLIST::iterator iKeyEvent;
typedef KEYLIST::const_iterator ciKeyEvent;
typedef KEYLIST::reverse_iterator riKeyEvent;
typedef KEYLIST::const_reverse_iterator criKeyEvent;



class KeyList : public KEYLIST {
      bool useList;
      key_enum _key;             // key if not using key list

      void add(unsigned tick, key_enum tempo);
      void change(unsigned tick, key_enum newKey);
      void del(iKeyEvent);
      void del(unsigned tick);

   public:

      KeyList();
      void clear();

      void read(Xml&);
      void write(int, Xml&) const;
      void dump() const;

      key_enum keyAtTick(unsigned tick) const;

      void addKey(unsigned t, key_enum newKey);
      void delKey(unsigned tick);
//      void changeKey(unsigned tick, key_enum newKey);
      bool setMasterFlag(unsigned tick, bool val);
      };

extern KeyList keymap;


#endif // KEYEVENT_H