package com.burn_preflight;
import processing.app.I18n;
import processing.app.PreferencesData;
import processing.app.debug.MessageConsumer;
import processing.app.debug.MessageSiphon;
import processing.app.debug.RunnerException;
import processing.app.helpers.ProcessUtils;
import processing.app.helpers.StringUtils;

import java.io.File;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

public class Command_Uploader implements MessageConsumer {

		public String Command_Uploader()throws Exception {
	    TargetPlatform targetPlatform = BaseNoGui.getTargetPlatform();
  System.out.println("Hello from CommandUpLoader");
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
	    if (programmerPrefs == null)
	      System.out.println("Please select a programmer from Tools->Programmer menu");

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
	      if (toolPrefs.size() == 0){}
	    }
	    toolPrefs.putAll(targetPlatform.getTool(tool));
	    if (toolPrefs.size() == 0){}

	    // Merge tool with global configuration
	    prefs.putAll(toolPrefs);
	    prefs.put("erase.verbose", prefs.getOrExcept("erase.params.verbose"));
	    prefs.put("bootloader.verbose", prefs.getOrExcept("bootloader.params.verbose"));
	    
	   String pattern = prefs.getOrExcept("bootloader.pattern");
	   String[] cmd = StringReplacer.formatAndSplit(pattern, prefs, true);
	    for(int i=0;i<cmd.length;i++){
	      System.out.println(cmd[i]+"\n");
	    }
	    return executeUploadCommand(cmd);
	  }

protected boolean executeUploadCommand(String command[]) throws Exception {
System.out.println("Hello from executeUploadCommand");
	    try {
	      System.out.println();
	      Process process = ProcessUtils.exec(command);
	      new MessageSiphon(process.getInputStream(), this, 100);
	      new MessageSiphon(process.getErrorStream(), this, 100);

	      // wait for the process to finish.
	      result = process.waitFor();

	    } catch (Exception e) {
	      e.printStackTrace();
	    }
return(false);
}
}
