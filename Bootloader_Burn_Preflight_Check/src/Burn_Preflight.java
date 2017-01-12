/* -*- mode: java; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*-------------------------------------------------------------------
Burn_Preflight.java 
Part of the Cestino Project
Copyright (c) 2015
James R. Strickland
---------------------------------------------------------------------
Sometimes Burn Bootloader will partially succeed, leaving the clock
fuses in an unknown state and the ATmega unable to start.
 
Since the Cestino project requires using Arduino as ISP (which can be
dodgy), and burning bootloaders of people who may not have a proper
ISP or EPROM burner to un-screw the fuses, we use AVRdude to read the
MPU status and fuse data in, and make sure it's valid before burning
the bootloader.

This tool is hacked together from a skeleton I derived from the Mangler
tool, and from SerialUploader.java, from the Arduino core itself. 
---------------------------------------------------------------------
License: GPL version 2

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
---------------------------------------------------------------------*/

package com.burn_preflight;

//These imports are required for our tool to show up in the tools menu.
import processing.app.*;
import processing.app.tools.Tool;

//We are extending SerialUploader, so we'd better include that, and its mama.
import cc.arduino.packages.uploaders.SerialUploader;
import cc.arduino.packages.Uploader;

//These imports are required for calling AVRDude and telling it what we want.
import processing.app.debug.RunnerException;
import processing.app.debug.TargetPlatform;
import processing.app.helpers.OSUtils;
import processing.app.helpers.PreferencesMap;
import processing.app.helpers.StringReplacer;

//And these are the usual Java utilities.
import java.io.File;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;



//So this class extends SerialUploader so we can access its private 
//methods and impliments tool so it shows up in the tools menu.
//what could possibly go wrong?
//-----------------------------------------------------------------
public class Burn_Preflight extends SerialUploader
							implements Tool {
  Editor editor;


  public void init(Editor editor) {
    this.editor = editor;
  }

//Actual title in the tools menu is set here.
  public String getMenuTitle() {
    return "Bootloader Burn Preflight Check";
  }

//This is called when you pick Bootloader Burn Preflight Check
//from the menu.
  public void run() {
        //All this just to say hello world.
        System.out.println("Hello from Burn_Preflight");

        try{
          //This method is the private one in SerialUploader.
        	if (executeUploadCommand(gencommand())){
        	} //we're not doing anything critical. If we catch exceptions, ignore them.        	
        	} catch (Exception e){
        	}
  }
  
  //The command we send to executeUploadCommand() is generated here. Ganked from 
  //SerialUploader and modified.
  //The way it works is: it returns an array of strings, each with part of the
  //command(s) that we're calling. Those are extracted from a variety of locations
  //for internationalization and platform flexibility.
  // Since this code is ganked from burnBootloader, but we don't want to mess with
  // the fuses or the bootloader, we then throw away the last two lines.
  //----------------------------------------------------------------------------
  public String[] gencommand() throws Exception {
	    TargetPlatform targetPlatform = BaseNoGui.getTargetPlatform();

	    // Find preferences for the selected programmer
	    PreferencesMap programmerPrefs;
	    String programmer = PreferencesData.get("programmer");
	    if (programmer.contains(":")) {
	      String[] split = programmer.split(":", 2);
	      TargetPlatform platform = BaseNoGui.getCurrentTargetPlatformFromPackage(split[0]);
	      programmer = split[1];
	      programmerPrefs = platform.getProgrammer(programmer);
	    } else {
	      programmerPrefs = targetPlatform.getProgrammer(programmer);
	    }

	    // Build configuration for the current programmer
	    PreferencesMap prefs = PreferencesData.getMap();
	    PreferencesMap boardPreferences = BaseNoGui.getBoardPreferences();
	    if (boardPreferences != null) {
	      prefs.putAll(boardPreferences);
	    }
	    prefs.putAll(programmerPrefs);

	    // Create configuration for bootloader tool
	    PreferencesMap toolPrefs = new PreferencesMap();
	    String tool = prefs.getOrExcept("bootloader.tool");
	    if (tool.contains(":")) {
	      String[] split = tool.split(":", 2);
	      TargetPlatform platform = BaseNoGui.getCurrentTargetPlatformFromPackage(split[0]);
	      tool = split[1];
	      toolPrefs.putAll(platform.getTool(tool));
	    }
	    toolPrefs.putAll(targetPlatform.getTool(tool));

	    // Merge tool with global configuration
	    prefs.putAll(toolPrefs);
	    prefs.put("erase.verbose", prefs.getOrExcept("erase.params.verbose"));
	    prefs.put("bootloader.verbose", prefs.getOrExcept("bootloader.params.verbose"));


	    String pattern = prefs.getOrExcept("bootloader.pattern");
	    
	    String[] command = StringReplacer.formatAndSplit(pattern, prefs, true);
	    //Here's where we make all this from the burn bootloader pattern to the preflight pattern.
	    command[command.length-1]="";
	    command[command.length-2]="";
	    return command;
	  }
}
