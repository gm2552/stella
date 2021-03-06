//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PointingDevice.hxx"
#include "SaveKey.hxx"
#include "AtariVox.hxx"
#include "Settings.hxx"
#include "EventMappingWidget.hxx"
#include "EditTextWidget.hxx"
#include "JoystickDialog.hxx"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "MessageBox.hxx"
#include "InputDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::InputDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Input settings"),
    myMaxWidth(max_w),
    myMaxHeight(max_h)
{
  const int lineHeight   = _font.getLineHeight(),
            fontWidth    = _font.getMaxCharWidth(),
            buttonHeight = _font.getLineHeight() + 4;
  const int vBorder = 4;
  int xpos, ypos, tabID;

  // Set real dimensions
  setSize(51 * fontWidth + 10, 17 * (lineHeight + 4) + 16 + _th, max_w, max_h);

  // The tab widget
  xpos = 2; ypos = vBorder + _th;
  myTab = new TabWidget(this, _font, xpos, ypos, _w - 2*xpos, _h -_th - buttonHeight - 20);
  addTabWidget(myTab);

  // 1) Event mapper for emulation actions
  tabID = myTab->addTab(" Emul. Events ", TabWidget::AUTO_WIDTH);
  myEmulEventMapper = new EventMappingWidget(myTab, _font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - 4,
                                             EventMode::kEmulationMode);
  myTab->setParentWidget(tabID, myEmulEventMapper);
  addToFocusList(myEmulEventMapper->getFocusList(), myTab, tabID);

  // 2) Event mapper for UI actions
  tabID = myTab->addTab(" UI Events ", TabWidget::AUTO_WIDTH);
  myMenuEventMapper = new EventMappingWidget(myTab, _font, 2, 2,
                                             myTab->getWidth(),
                                             myTab->getHeight() - 4,
                                             EventMode::kMenuMode);
  myTab->setParentWidget(tabID, myMenuEventMapper);
  addToFocusList(myMenuEventMapper->getFocusList(), myTab, tabID);

  // 3) Devices & ports
  addDevicePortTab();

  // 4) Mouse
  addMouseTab();

  // Finalize the tabs, and activate the first tab
  myTab->activateTabs();
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
InputDialog::~InputDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addDevicePortTab()
{
  const int lineHeight = _font.getLineHeight(),
            fontWidth  = _font.getMaxCharWidth(),
            fontHeight = _font.getFontHeight();
  int xpos, ypos, lwidth, tabID;
  WidgetArray wid;
  const int VGAP = 4;
  const int VBORDER = 8;
  const int HBORDER = 8;

  // Devices/ports
  tabID = myTab->addTab("Devices & Ports", TabWidget::AUTO_WIDTH);

  ypos = VBORDER;
  lwidth = _font.getStringWidth("Digital paddle sensitivity ");

  // Add joystick deadzone setting
  myDeadzone = new SliderWidget(myTab, _font, HBORDER, ypos - 1, 13 * fontWidth, lineHeight,
                                "Joystick deadzone size", lwidth, kDeadzoneChanged, 5 * fontWidth);
  myDeadzone->setMinValue(0); myDeadzone->setMaxValue(29);
  myDeadzone->setTickmarkIntervals(4);
  xpos = HBORDER + myDeadzone->getWidth() + 5;
  wid.push_back(myDeadzone);

  xpos = HBORDER; ypos += lineHeight + VGAP * 2;
  new StaticTextWidget(myTab, _font, xpos, ypos+1, "Analog paddle:");

  // Add paddle center
  xpos += fontWidth * 2;
  ypos += lineHeight + VGAP;

  myPaddleCenter = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Center",
                                    lwidth - fontWidth * 2, kPCenterChanged, 6 * fontWidth, "px", 0, true);
  myPaddleCenter->setMinValue(Paddles::MIN_ANALOG_CENTER);
  myPaddleCenter->setMaxValue(Paddles::MAX_ANALOG_CENTER);
  myPaddleCenter->setTickmarkIntervals(4);
  wid.push_back(myPaddleCenter);

  // Add paddle sensitivity
  ypos += lineHeight + VGAP;
  myPaddleSpeed = new SliderWidget(myTab, _font, xpos, ypos - 1, 13 * fontWidth, lineHeight,
                                   "Sensitivity",
                                   lwidth - fontWidth * 2, kPSpeedChanged, 4 * fontWidth, "%");
  myPaddleSpeed->setMinValue(0); myPaddleSpeed->setMaxValue(Paddles::MAX_ANALOG_SENSE);
  myPaddleSpeed->setTickmarkIntervals(3);
  wid.push_back(myPaddleSpeed);


  // Add dejitter (Stelladaptor emulation for now only)
  ypos += lineHeight + VGAP;
  myDejitterBase = new SliderWidget(myTab, _font, xpos, ypos - 1, 6 * fontWidth, lineHeight,
                                    "Dejitter strength", lwidth - fontWidth * 2, kDejitterChanged);
  myDejitterBase->setMinValue(Paddles::MIN_DEJITTER);
  myDejitterBase->setMaxValue(Paddles::MAX_DEJITTER);
  myDejitterBase->setTickmarkIntervals(2);
  xpos += myDejitterBase->getWidth() + fontWidth - 4;
  wid.push_back(myDejitterBase);

  myDejitterDiff = new SliderWidget(myTab, _font, xpos, ypos - 1, 6 * fontWidth, lineHeight,
                                    "", 0, kDejitterChanged);
  myDejitterDiff->setMinValue(Paddles::MIN_DEJITTER);
  myDejitterDiff->setMaxValue(Paddles::MAX_DEJITTER);
  myDejitterDiff->setTickmarkIntervals(2);
  xpos += myDejitterDiff->getWidth();
  wid.push_back(myDejitterDiff);

  myDejitterLabel = new StaticTextWidget(myTab, _font, xpos, ypos + 1, 7 * fontWidth, lineHeight, "");

  // Add paddle speed (digital emulation)
  ypos += lineHeight + VGAP * 4;
  myDPaddleSpeed = new SliderWidget(myTab, _font, HBORDER, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Digital paddle sensitivity",
                                    lwidth, kDPSpeedChanged, 4 * fontWidth, "%");
  myDPaddleSpeed->setMinValue(1); myDPaddleSpeed->setMaxValue(20);
  myDPaddleSpeed->setTickmarkIntervals(4);
  wid.push_back(myDPaddleSpeed);

  // Add trackball speed
  ypos += lineHeight + VGAP * 2;
  myTrackBallSpeed = new SliderWidget(myTab, _font, HBORDER, ypos - 1, 13 * fontWidth, lineHeight,
                                      "Trackball sensitivity",
                                      lwidth, kTBSpeedChanged, 4 * fontWidth, "%");
  myTrackBallSpeed->setMinValue(1); myTrackBallSpeed->setMaxValue(20);
  myTrackBallSpeed->setTickmarkIntervals(4);
  wid.push_back(myTrackBallSpeed);

  // Add 'allow all 4 directions' for joystick
  ypos += lineHeight + VGAP * 4;
  myAllowAll4 = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                  "Allow all 4 directions on joystick");
  wid.push_back(myAllowAll4);

  // Enable/disable modifier key-combos
  ypos += lineHeight + VGAP;
  myModCombo = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                  "Use modifier key combos");
  wid.push_back(myModCombo);
  ypos += lineHeight + VGAP;

  // Stelladaptor mappings
  mySAPort = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                                "Swap Stelladaptor ports");
  wid.push_back(mySAPort);

  int fwidth;

  // Add EEPROM erase (part 1/2)
  ypos += VGAP*4;
  fwidth = _font.getStringWidth("AtariVox/SaveKey");
  lwidth = _font.getStringWidth("AtariVox/SaveKey");
  new StaticTextWidget(myTab, _font, _w - HBORDER - 4 - (fwidth + lwidth) / 2, ypos,
                       "AtariVox/SaveKey");

  // Show joystick database
  ypos += lineHeight;
  myJoyDlgButton = new ButtonWidget(myTab, _font, HBORDER, ypos, 20,
    "Joystick Database" + ELLIPSIS, kDBButtonPressed);
  wid.push_back(myJoyDlgButton);

  // Add EEPROM erase (part 1/2)
  myEraseEEPROMButton = new ButtonWidget(myTab, _font, _w - HBORDER - 4 - fwidth, ypos,
                                         fwidth, lineHeight+4,
                                        "Erase EEPROM", kEEButtonPressed);
  wid.push_back(myEraseEEPROMButton);

  // Add AtariVox serial port
  ypos += lineHeight + VGAP * 2;
  lwidth = _font.getStringWidth("AVox serial port ");
  fwidth = _w - HBORDER * 2 - 4 - lwidth;
  new StaticTextWidget(myTab, _font, HBORDER, ypos + 2, "AVox serial port ");
  myAVoxPort = new EditTextWidget(myTab, _font, HBORDER + lwidth, ypos,
                                  fwidth, fontHeight);
  wid.push_back(myAVoxPort);

  // Add items for virtual device ports
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::addMouseTab()
{
  const int lineHeight = _font.getLineHeight(),
    fontWidth  = _font.getMaxCharWidth(),
    fontHeight = _font.getFontHeight();
  int ypos, lwidth, pwidth, tabID;
  WidgetArray wid;
  VariantList items;
  const int VGAP = 4;
  const int VBORDER = 8;
  const int HBORDER = 8;

  // Mouse
  tabID = myTab->addTab(" Mouse ", TabWidget::AUTO_WIDTH);

  ypos = VBORDER;
  lwidth = _font.getStringWidth("Use mouse as a controller ");
  pwidth = _font.getStringWidth("-UI, -Emulation");

  // Use mouse as controller
  VarList::push_back(items, "Always", "always");
  VarList::push_back(items, "Analog devices", "analog");
  VarList::push_back(items, "Never", "never");
  myMouseControl = new PopUpWidget(myTab, _font, HBORDER, ypos, pwidth, lineHeight, items,
                                   "Use mouse as a controller ", lwidth, kMouseCtrlChanged);
  wid.push_back(myMouseControl);

  // Add paddle speed (mouse emulation)
  ypos += lineHeight + VGAP;
  myMPaddleSpeed = new SliderWidget(myTab, _font, HBORDER, ypos - 1, 13 * fontWidth, lineHeight,
                                    "Mouse paddle sensitivity ",
                                    lwidth, kMPSpeedChanged, 4 * fontWidth, "%");
  myMPaddleSpeed->setMinValue(1); myMPaddleSpeed->setMaxValue(20);
  myMPaddleSpeed->setTickmarkIntervals(4);
  wid.push_back(myMPaddleSpeed);


  // Mouse cursor state
  ypos += lineHeight + VGAP * 4;
  items.clear();
  VarList::push_back(items, "-UI, -Emulation", "0");
  VarList::push_back(items, "-UI, +Emulation", "1");
  VarList::push_back(items, "+UI, -Emulation", "2");
  VarList::push_back(items, "+UI, +Emulation", "3");
  myCursorState = new PopUpWidget(myTab, _font, HBORDER, ypos, pwidth, lineHeight, items,
                                  "Mouse cursor visibility ", lwidth, kCursorStateChanged);
  wid.push_back(myCursorState);
#ifndef WINDOWED_SUPPORT
  myCursorState->clearFlags(Widget::FLAG_ENABLED);
#endif

  // Grab mouse (in windowed mode)
  ypos += lineHeight + VGAP;
  myGrabMouse = new CheckboxWidget(myTab, _font, HBORDER, ypos,
                                   "Grab mouse in emulation mode");
  wid.push_back(myGrabMouse);
#ifndef WINDOWED_SUPPORT
  myGrabMouse->clearFlags(Widget::FLAG_ENABLED);
#endif

  // Add items for mouse
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::loadConfig()
{
  // Left & right ports
  mySAPort->setState(instance().settings().getString("saport") == "rl");

  // Use mouse as a controller
  myMouseControl->setSelected(
    instance().settings().getString("usemouse"), "analog");
  handleMouseControlState();

  // Mouse cursor state
  myCursorState->setSelected(instance().settings().getString("cursor"), "2");
  handleCursorState();

  // Joystick deadzone
  myDeadzone->setValue(instance().settings().getInt("joydeadzone"));

  // Paddle center & speed (analog)
  myPaddleCenter->setValue(instance().settings().getInt("pcenter"));
  myPaddleSpeed->setValue(instance().settings().getInt("psense"));

  // Paddle speed (digital and mouse)
  myDejitterBase->setValue(instance().settings().getInt("dejitter.base"));
  myDejitterDiff->setValue(instance().settings().getInt("dejitter.diff"));
  updateDejitter();
  myDPaddleSpeed->setValue(instance().settings().getInt("dsense"));
  myMPaddleSpeed->setValue(instance().settings().getInt("msense"));

  // Trackball speed
  myTrackBallSpeed->setValue(instance().settings().getInt("tsense"));

  // AtariVox serial port
  myAVoxPort->setText(instance().settings().getString("avoxport"));

  // EEPROM erase (only enable in emulation mode and for valid controllers)
  if(instance().hasConsole())
  {
    Controller& lport = instance().console().leftController();
    Controller& rport = instance().console().rightController();

    myEraseEEPROMButton->setEnabled(lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox ||
                                    rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox);
  }
  else
    myEraseEEPROMButton->setEnabled(false);

  // Allow all 4 joystick directions
  myAllowAll4->setState(instance().settings().getBool("joyallow4"));

  // Grab mouse
  myGrabMouse->setState(instance().settings().getBool("grabmouse"));

  // Enable/disable modifier key-combos
  myModCombo->setState(instance().settings().getBool("modcombo"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::saveConfig()
{
  // Left & right ports
  instance().eventHandler().mapStelladaptors(mySAPort->getState() ? "rl": "lr");

  // Use mouse as a controller
  const string& usemouse = myMouseControl->getSelectedTag().toString();
  instance().settings().setValue("usemouse", usemouse);
  instance().eventHandler().setMouseControllerMode(usemouse);

  // Joystick deadzone
  int deadzone = myDeadzone->getValue();
  instance().settings().setValue("joydeadzone", deadzone);
  Joystick::setDeadZone(deadzone);

  // Paddle center (analog)
  int center = myPaddleCenter->getValue();
  instance().settings().setValue("pcenter", center);
  Paddles::setAnalogCenter(center);

  // Paddle speed (analog)
  int sensitivity = myPaddleSpeed->getValue();
  instance().settings().setValue("psense", sensitivity);
  Paddles::setAnalogSensitivity(sensitivity);

  // Paddle speed (digital and mouse)
  int dejitter = myDejitterBase->getValue();
  instance().settings().setValue("dejitter.base", dejitter);
  Paddles::setDejitterBase(dejitter);
  dejitter = myDejitterDiff->getValue();
  instance().settings().setValue("dejitter.diff", dejitter);
  Paddles::setDejitterDiff(dejitter);

  sensitivity = myDPaddleSpeed->getValue();
  instance().settings().setValue("dsense", sensitivity);
  Paddles::setDigitalSensitivity(sensitivity);

  sensitivity = myMPaddleSpeed->getValue();
  instance().settings().setValue("msense", sensitivity);
  Paddles::setMouseSensitivity(sensitivity);

  // Trackball speed
  sensitivity = myTrackBallSpeed->getValue();
  instance().settings().setValue("tsense", sensitivity);
  PointingDevice::setSensitivity(sensitivity);

  // AtariVox serial port
  instance().settings().setValue("avoxport", myAVoxPort->getText());

  // Allow all 4 joystick directions
  bool allowall4 = myAllowAll4->getState();
  instance().settings().setValue("joyallow4", allowall4);
  instance().eventHandler().allowAllDirections(allowall4);

  // Grab mouse and hide cursor
  const string& cursor = myCursorState->getSelectedTag().toString();
  instance().settings().setValue("cursor", cursor);
  // only allow grab mouse if cursor is hidden in emulation
  int state = myCursorState->getSelected();
  bool enableGrab = state != 1 && state != 3;
  bool grab = enableGrab ? myGrabMouse->getState() : false;
  instance().settings().setValue("grabmouse", grab);
  instance().frameBuffer().enableGrabMouse(grab);

  // Enable/disable modifier key-combos
  instance().settings().setValue("modcombo", myModCombo->getState());

  instance().eventHandler().saveKeyMapping();
  instance().eventHandler().saveJoyMapping();
//  instance().saveConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // Emulation events
      myEmulEventMapper->setDefaults();
      break;

    case 1:  // UI events
      myMenuEventMapper->setDefaults();
      break;

    case 2:  // Devices & Ports
      // Left & right ports
      mySAPort->setState(false);

      // Joystick deadzone
      myDeadzone->setValue(0);

      // Paddle center & speed (analog)
      myPaddleCenter->setValue(0);
      myPaddleSpeed->setValue(20);

      // Paddle speed (digital)
      myDPaddleSpeed->setValue(10);
    #if defined(RETRON77)
      myDejitterBase->setValue(2);
      myDejitterDiff->setValue(6);
    #else
      myDejitterBase->setValue(0);
      myDejitterDiff->setValue(0);
    #endif
      updateDejitter();
      myTrackBallSpeed->setValue(10);

      // AtariVox serial port
      myAVoxPort->setText("");

      // Allow all 4 joystick directions
      myAllowAll4->setState(false);

      // Enable/disable modifier key-combos
      myModCombo->setState(true);

      break;

    case 3:  // Mouse
      // Use mouse as a controller
      myMouseControl->setSelected("analog");

      // Mouse cursor state
      myCursorState->setSelected("2");

      // Grab mouse
      myGrabMouse->setState(true);

      // Paddle speed (mouse)
      myMPaddleSpeed->setValue(10);

      handleMouseControlState();
      handleCursorState();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::repeatEnabled()
{
  return !myEmulEventMapper->isRemapping() && !myMenuEventMapper->isRemapping();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyDown(StellaKey key, StellaMod mod, bool repeated)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if (myEmulEventMapper->remapMode())
    myEmulEventMapper->handleKeyDown(key, mod);
  else if (myMenuEventMapper->remapMode())
    myMenuEventMapper->handleKeyDown(key, mod);
  else
    Dialog::handleKeyDown(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleKeyUp(StellaKey key, StellaMod mod)
{
  // Remap key events in remap mode, otherwise pass to parent dialog
  if (myEmulEventMapper->remapMode())
    myEmulEventMapper->handleKeyUp(key, mod);
  else if (myMenuEventMapper->remapMode())
    myMenuEventMapper->handleKeyUp(key, mod);
  else
    Dialog::handleKeyUp(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyDown(int stick, int button, bool longPress)
{
  // Remap joystick buttons in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    myEmulEventMapper->handleJoyDown(stick, button);
  else if(myMenuEventMapper->remapMode())
    myMenuEventMapper->handleJoyDown(stick, button);
  else
    Dialog::handleJoyDown(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyUp(int stick, int button)
{
  // Remap joystick buttons in remap mode, otherwise pass to parent dialog
  if (myEmulEventMapper->remapMode())
    myEmulEventMapper->handleJoyUp(stick, button);
  else if (myMenuEventMapper->remapMode())
    myMenuEventMapper->handleJoyUp(stick, button);
  else
    Dialog::handleJoyUp(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button)
{
  // Remap joystick axis in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    myEmulEventMapper->handleJoyAxis(stick, axis, adir, button);
  else if(myMenuEventMapper->remapMode())
    myMenuEventMapper->handleJoyAxis(stick, axis, adir, button);
  else
    Dialog::handleJoyAxis(stick, axis, adir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InputDialog::handleJoyHat(int stick, int hat, JoyHatDir hdir, int button)
{
  // Remap joystick hat in remap mode, otherwise pass to parent dialog
  if(myEmulEventMapper->remapMode())
    return myEmulEventMapper->handleJoyHat(stick, hat, hdir, button);
  else if(myMenuEventMapper->remapMode())
    return myMenuEventMapper->handleJoyHat(stick, hat, hdir, button);
  else
    return Dialog::handleJoyHat(stick, hat, hdir, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::eraseEEPROM()
{
  // This method will only be callable if a console exists, so we don't
  // need to check again here
  Controller& lport = instance().console().leftController();
  Controller& rport = instance().console().rightController();

  if(lport.type() == Controller::Type::SaveKey || lport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(lport);
    skey.eraseCurrent();
  }

  if(rport.type() == Controller::Type::SaveKey || rport.type() == Controller::Type::AtariVox)
  {
    SaveKey& skey = static_cast<SaveKey&>(rport);
    skey.eraseCurrent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kCloseCmd:
      // Revert changes made to event mapping
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kDeadzoneChanged:
      myDeadzone->setValueLabel(3200 + 1000 * myDeadzone->getValue());
      break;

    case kPCenterChanged:
      myPaddleCenter->setValueLabel(myPaddleCenter->getValue() * 5);
      break;

    case kPSpeedChanged:
      myPaddleSpeed->setValueLabel(Paddles::setAnalogSensitivity(myPaddleSpeed->getValue()) * 100.0 + 0.5);
      break;

    case kDejitterChanged:
      updateDejitter();
      break;

    case kDPSpeedChanged:
      myDPaddleSpeed->setValueLabel(myDPaddleSpeed->getValue() * 10);
      break;

    case kTBSpeedChanged:
      myTrackBallSpeed->setValueLabel(myTrackBallSpeed->getValue() * 10);
      break;

    case kDBButtonPressed:
      if(!myJoyDialog)
      {
        const GUI::Font& font = instance().frameBuffer().font();
        myJoyDialog = make_unique<JoystickDialog>
          (this, font, font.getMaxCharWidth() * 56 + 20, font.getFontHeight() * 18 + 20);
      }
      myJoyDialog->show();
      break;

    case kEEButtonPressed:
      if(!myConfirmMsg)
      {
        StringList msg;
        msg.push_back("This operation cannot be undone.");
        msg.push_back("All data stored on your AtariVox");
        msg.push_back("or SaveKey will be erased!");
        msg.push_back("");
        msg.push_back("If you are sure you want to erase");
        msg.push_back("the data, click 'OK', otherwise ");
        msg.push_back("click 'Cancel'.");
        myConfirmMsg = make_unique<GUI::MessageBox>
          (this, instance().frameBuffer().font(), msg,
           myMaxWidth, myMaxHeight, kConfirmEEEraseCmd,
           "OK", "Cancel", "Erase EEPROM", false);
      }
      myConfirmMsg->show();
      break;

    case kConfirmEEEraseCmd:
      eraseEEPROM();
      break;

    case kMouseCtrlChanged:
      handleMouseControlState();
      break;

    case kCursorStateChanged:
      handleCursorState();
      break;

    case kMPSpeedChanged:
      myMPaddleSpeed->setValueLabel(myMPaddleSpeed->getValue() * 10);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::updateDejitter()
{
  int strength = myDejitterBase->getValue();
  stringstream label;

  if (strength)
    label << myDejitterBase->getValue();
  else
    label << "Off";

  label << " ";
  strength = myDejitterDiff->getValue();

  if (strength)
    label << myDejitterDiff->getValue();
  else
    label << "Off";

  myDejitterLabel->setLabel(label.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleMouseControlState()
{
  myMPaddleSpeed->setEnabled(myMouseControl->getSelected() != 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InputDialog::handleCursorState()
{
  int state = myCursorState->getSelected();
  bool enableGrab = state != 1 && state != 3;

  myGrabMouse->setEnabled(enableGrab);
}
