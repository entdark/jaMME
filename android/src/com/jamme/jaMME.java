package com.jamme;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.regex.Pattern;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import org.libsdl.app.SDLActivity;

import com.jamme.DirectoryChooserDialog;
import com.jamme.R;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.EGLConfigChooser;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class jaMME extends Activity {
	private Activity act;
	private Vibrator v;
	private float density = 1.0f;
	private boolean gameReady = false;
	private String gamePath, gameArgs = "", demoName = null;
	
	private jaMMEView view;
	private jaMMERenderer renderer = new jaMMERenderer();
	private jaMMEEditText et;
	private jaMMESeekBar sb = null;
	private jaMMERangeSeekBar rsb = null;
	
	private int surfaceWidth, surfaceHeight;

	int flags = 0;
	int CONSOLE_ACTIVE				= 1<<2;
	int CONSOLE_ACTIVE_FULLSCREEN	= 1<<3;
	int UI_ACTIVE					= 1<<4;
	int UI_EDITINGFIELD				= 1<<5;
	int DEMO_PLAYBACK				= 1<<6;
	int DEMO_PAUSED					= 1<<7;
	int CHAT_SAY					= 1<<8;
	int CHAT_SAY_TEAM				= 1<<9;
	int INGAME						= ~(CONSOLE_ACTIVE
										| CONSOLE_ACTIVE_FULLSCREEN
										| UI_ACTIVE
										| UI_EDITINGFIELD | DEMO_PLAYBACK);
//	int DEMO_CUTTING				= 1<<17;
//	boolean demoCutting = false;
	boolean keyboardActive = false;
	private static void loadLibraries() {
		System.loadLibrary("SDL2");
        System.loadLibrary("jamme");
    }
	
	private static native int init(Activity act,String[] args,String game_path,String lib_path,String app_path,String abi,String abi_alt);
	private static native void setScreenSize(int width, int height);	
	private static native int frame();
	private static native String getLoadingMsg();
	private static native void keypress(int down, int qkey, int unicode);
	private static native void textPaste(byte []paste);
	private static native void doAction(int state, int action);
	private static native void analogPitch(int mode,float v);
	private static native void analogYaw(int mode,float v);
	private static native void mouseMove(float dx, float dy);
	private static native void quickCommand(String command);
	
	/* DEMO STUFF */
	private static native int demoGetLength();
	private static native int demoGetTime();
	/* FEEDBACK */
	private static native int vibrateFeedback();
	private void vibrate() {
		long time = vibrateFeedback();
		if (time > 5)
			v.vibrate(time);
	}
    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        act = this;
        v = (Vibrator)getSystemService(Context.VIBRATOR_SERVICE);
        density = getResources().getDisplayMetrics().density;
		// fullscreen
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		// keep screen on 
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
				WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		loadLibraries();
		view = new jaMMEView(this);
		view.setEGLConfigChooser(new BestEglChooser(getApplicationContext()));
        view.setRenderer(renderer);
        view.setKeepScreenOn(true);
        setContentView(view);
        view.requestFocus();
        view.setFocusableInTouchMode(true);
        String[] parameters = {"","",""};
		try {
			parameters = loadParameters();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		String baseDirectory = "";
		if (parameters.length > 0)
			baseDirectory = parameters[0];
        String argsStr = "";
		if (parameters.length > 1)
			argsStr = parameters[1];
		boolean logSave = false;
		if (parameters.length > 2)
			logSave = Boolean.parseBoolean(parameters[2]);
		try {
			if (checkAssets(baseDirectory) && openDemo()) {
				gameReady = true;
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if (!gameReady) {
			addLauncherItems(this, baseDirectory, argsStr, logSave);
		}
		act.setTheme(android.R.style.Theme_Holo);
    }
	@Override
	protected void onPause() {
		Log.i("jaMME", "onPause");
		super.onPause();
//		view.onPause();
		if (gameReady) {
			SDLActivity.nativePause();
		}
	}
	@Override
	protected void onResume() {
		Log.i("jaMME", "onResume");
    	super.onResume();
        view.onResume();
		if (gameReady) {
			SDLActivity.nativeResume();
		}
	}
	@Override
	protected void onRestart() {
		Log.i("jaMME", "onRestart");
		super.onRestart();
	}
	@Override
	protected void onStop() {
		Log.i("jaMME", "onStop");
		super.onStop();	
	}
	@Override
	protected void onDestroy() {
		Log.i("jaMME", "onDestroy");
		super.onDestroy();
		if (!gameReady) {
			return;
		}
		//stop demo to be able to remove currently opened demo file
		keypress(1, JAKeycodes.A_ESCAPE.ordinal(), 0);
		keypress(0, JAKeycodes.A_ESCAPE.ordinal(), 0);
		do {
			flags = frame();
		} while((flags & DEMO_PLAYBACK) != 0);
		
		SDLActivity.nativeQuit();
		
		//if user closed the application from OS side on the demo playback after opening them externally
		if (gamePath != null && demoName != null) {
			int i = 0;
			while(demoExtList[i] != null) {
				String demosFolder = gamePath + "/base/demos/";
        		if (demoName.contains(".mme"))
        			demosFolder = gamePath + "/base/mmedemos/";
				File demo = new File(demosFolder + demoName + demoExtList[i]);
				Log.i("jaMME", "onDestroy: removing " + demo.toString());
				if (demo.exists()) {
					Log.i("jaMME", "onDestroy: removed");
			    	demo.delete();
				}
				i++;
			}
		}
		System.exit(0);
	}
	//gay way, but fastest
	private static boolean equals(final String s1, final String s2) {
		return s1.length() == s2.length() && s1.hashCode() == s2.hashCode();
	}
	private boolean openDemo() throws IOException {
		Log.i("jaMME", "openDemo");
		Intent intent = getIntent();
		if (intent == null) {
			return false;
		}
        String action = intent.getAction();
    	Uri uri = intent.getData();
        if (action != null && uri != null && action.compareTo(Intent.ACTION_VIEW) == 0) {
        	String demo = java.net.URLDecoder.decode((uri.getEncodedPath()), "UTF-8");
        	int i = 0;
        	while(demoExtList[i] != null) {
        		if (demo.contains(demoExtList[i]))
        			break;
        		i++;
			}
        	if (demoExtList[i] != null) {
        		String demosFolder = gamePath + "/base/demos/";
        		if (demo.contains(".mme"))
        			demosFolder = gamePath + "/base/mmedemos/";
		    	File from = new File(demo);
		    	File to = new File(demosFolder + from.getName());
		    	while (to.exists()) {
		        	to = new File(demosFolder + "_" + to.getName());
		    	}
		    	copyFile(from, to);
		    	demoName = to.getName();
		    	int pos = demoName.lastIndexOf(".");
		    	if (pos > 0) {
		    		demoName = demoName.substring(0, pos);
		    	}
				gameArgs = "+demo \"" + demoName + "\" del";
		    	return true;
        	}
        }
        return false;
	}
	private static void copyFile(File sourceFile, File destFile) throws IOException {
	    if (!destFile.exists()) {
	        destFile.createNewFile();
	    }
	    FileChannel source = null;
	    FileChannel destination = null;
	    try {
	        source = new FileInputStream(sourceFile).getChannel();
	        destination = new FileOutputStream(destFile).getChannel();
	        destination.transferFrom(source, 0, source.size());
	    } finally {
	        if (source != null) {
	            source.close();
	        }
	        if (destination != null) {
	            destination.close();
	        }
	    }
	}
	RelativeLayout layout;
	EditText args, baseDirET;
	Button startGame, baseDir;
	CheckBox logSave;
	private void addLauncherItems(Context context, String baseDirectory, String argsStr, boolean saveLog) {
		float d = density;
/* LAYOUT */
		Drawable bg = getResources().getDrawable(R.drawable.jamme);
		bg.setAlpha(0x17);
		layout = new RelativeLayout(this);
		layout.setBackgroundColor(Color.argb(0xFF, 0x13, 0x13, 0x13));
		layout.setBackground(bg);
/* ARGS */
		args = new EditText(context);
		args.setMaxLines(1);
		args.setInputType(EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_SHORT_MESSAGE);
		args.setHint("+startup args");
		args.setHintTextColor(Color.GRAY);
		args.setText(argsStr);
		args.setTextColor(Color.argb(0xFF, 0xC3, 0x00, 0x00));
		args.setBackground(getResources().getDrawable(R.drawable.jamme_edit_text));
		args.clearFocus();
		args.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
		args.setId(5);
		RelativeLayout.LayoutParams argsParams = 
		        new RelativeLayout.LayoutParams(
		        	ViewGroup.LayoutParams.MATCH_PARENT, 
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		argsParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		argsParams.addRule(RelativeLayout.CENTER_VERTICAL);
/* START */
		startGame = new Button(context);
		startGame.setBackground(getResources().getDrawable(R.drawable.jamme_btn));
		startGame.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				String gp = "";
				if (baseDirET != null && baseDirET.getText() != null && baseDirET.getText().toString() != null)
					gp = baseDirET.getText().toString();
				if (checkAssets(gp)) {
					if (args != null && args.getText() != null && args.getText().toString() != null)
						gameArgs = args.getText().toString();
					logging = logSave.isChecked();
					String []parameters = {gp,gameArgs,String.valueOf(logging)};
					try {
						saveParameters(parameters);
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					removeView(startGame);
					removeView(args);
					removeView(baseDirET);
					removeView(baseDir);
					removeView(logSave);
					removeView(layout);
					gameReady = true;
				} else {
					AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(act);
					alertDialogBuilder.setTitle("Missing assets[0-3].pk3");
					alertDialogBuilder
						.setMessage("Select base folder where assets[0-3].pk3 are placed in")
						.setNeutralButton("Ok", null);
					AlertDialog alertDialog = alertDialogBuilder.create();
					alertDialog.show();
				}
			}
		});
		startGame.setText("Start");
		RelativeLayout.LayoutParams startParams = 
		        new RelativeLayout.LayoutParams(
		            RelativeLayout.LayoutParams.WRAP_CONTENT,   
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		startParams.addRule(RelativeLayout.BELOW, args.getId());
		startParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		startParams.setMargins(0, (int)(3.5f*d), 0, 0);
/* BASE DIRECTORY EDITABLE */
		baseDirET = new EditText(context);
		baseDirET.setMaxLines(1);
		baseDirET.setInputType(EditorInfo.TYPE_CLASS_TEXT | EditorInfo.TYPE_TEXT_VARIATION_SHORT_MESSAGE);
		baseDirET.setHint("game directory");
		baseDirET.setHintTextColor(Color.GRAY);
		baseDirET.setText(baseDirectory);
		baseDirET.setTextColor(Color.argb(0xFF, 0xC3, 0x00, 0x00));
		baseDirET.setBackground(getResources().getDrawable(R.drawable.jamme_edit_text));
		baseDirET.clearFocus();
		baseDirET.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
		baseDirET.setId(17);
		RelativeLayout.LayoutParams baseDirETParams = 
		        new RelativeLayout.LayoutParams(
		        	ViewGroup.LayoutParams.MATCH_PARENT, 
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		baseDirETParams.addRule(RelativeLayout.ABOVE, args.getId());
		baseDirETParams.addRule(RelativeLayout.CENTER_VERTICAL);
		baseDirETParams.setMargins(0, 0, 0, (int)(3.5f*d));
/* BASE DIRECTORY */
		baseDir = new Button(context);
		baseDir.setBackground(getResources().getDrawable(R.drawable.jamme_btn));
		baseDir.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				DirectoryChooserDialog directoryChooserDialog = 
					new DirectoryChooserDialog(act, new DirectoryChooserDialog.ChosenDirectoryListener() {
						@Override
						public void onChosenDir(String chosenDir) {
							if (chosenDir != null)
								baseDirET.setText(chosenDir);
							else
								baseDirET.setText("");
						}
					});
				String chooseDir = "";
				if (baseDirET != null && baseDirET.getText() != null && baseDirET.getText().toString() != null)
					chooseDir = baseDirET.getText().toString();
				directoryChooserDialog.chooseDirectory(chooseDir);
			}
		});
		baseDir.setText("Game Directory");
		RelativeLayout.LayoutParams baseDirParams = 
		        new RelativeLayout.LayoutParams(
		            RelativeLayout.LayoutParams.WRAP_CONTENT,   
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		baseDirParams.addRule(RelativeLayout.ABOVE, baseDirET.getId());
		baseDirParams.addRule(RelativeLayout.CENTER_HORIZONTAL);
		baseDirParams.setMargins(0, 0, 0, (int)(3.5f*d));
/* LOGGING */
		logSave = new CheckBox(context);
		logSave.setChecked(saveLog);
		logSave.setButtonDrawable(getResources().getDrawable(R.drawable.jamme_checkbox));
		logSave.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				logging = isChecked;
			}
		});
		logSave.setText("Save Log");
		logSave.setTextColor(Color.argb(0xFF, 0xC3, 0x00, 0x00));
		RelativeLayout.LayoutParams logSaveParams = 
		        new RelativeLayout.LayoutParams(
		            RelativeLayout.LayoutParams.WRAP_CONTENT,   
		            RelativeLayout.LayoutParams.WRAP_CONTENT);
		logSaveParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
		logSaveParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
/* ADD TO LAYOUT */
		layout.addView(args, argsParams);
		layout.addView(startGame, startParams);
		layout.addView(baseDirET, baseDirETParams);
		layout.addView(baseDir, baseDirParams);
		layout.addView(logSave, logSaveParams);
		addContentView(layout, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
	}
	private static String getRemovableStorage() {
		String value = System.getenv("SECONDARY_STORAGE");
        if (!TextUtils.isEmpty(value)) {
            String[] paths = value.split(":");
            for (String path : paths) {
            	File file = new File(path);
                if (file.isDirectory()) {
                	return file.toString();
                }
            }
        }
        return null; // Most likely, a removable micro sdcard doesn't exist
	}
	private boolean checkAssets(String path) {
		if (path == null)
			return false;
		ArrayList<String> dirs = new ArrayList<String>();
		dirs.add(path);
		if (path.endsWith("/base")) {
			int l = path.lastIndexOf("/base");
			String baseRoot = path.substring(0, l);
			dirs.add(baseRoot);
		}
		for (Object d: dirs.toArray()) {
			File dir = new File((String)d + "/base");
			if(dir.exists() && dir.isDirectory()) {
				for (int i = 0; i < 3; i++) {
					File assets = new File((String)d + "/base/assets" + i + ".pk3");
					if (!assets.exists()) {
						return false;
					}
				}
				gamePath = (String)d;
				return true;
			}
		}
		return false;
	}
	private String []loadParameters() throws IOException {
		String []ret = {"",""};
		File f = new File(this.getFilesDir().toString() + "/parameters");
		if (f.exists()) {
			int length = (int)f.length();
			byte []buffer = new byte[length];
			FileInputStream in = new FileInputStream(f);
			try {
			    in.read(buffer);
			} finally {
			    in.close();
			}
			String s = new String(buffer);
			ret = s.split(Pattern.quote("\n\n\n\n\n"));
		}
		return ret;
	}
	private void saveParameters(String []parameters) throws IOException {
		File f = new File(this.getFilesDir().toString() + "/parameters");
		FileOutputStream stream = new FileOutputStream(f);
		try {
		    for (String p: parameters) {
		    	String s = p+"\n\n\n\n\n";
		    	stream.write(s.getBytes());
		    }
		} finally {
		    stream.close();
		}
	}
	private boolean ignoreAfter = false;
	private int prevLength = 0;
	private class jaMMEEditText extends EditText {
		public jaMMEEditText(Context context) {
			super(context);
			setY(surfaceHeight + 32);
			setX(surfaceWidth + 32);
			ignoreAfter = false;
			prevLength = 0;
			setAlpha(0.0f);
			addTextChangedListener(new jaMMETextWatcher());
	        clearFocus();
	        setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
	        setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
	        setOnEditorActionListener(new EditText.OnEditorActionListener() {
	            @Override
	            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
	                if (event == null)
	                	return false;
	            	int uc = event.getUnicodeChar(event.getMetaState());
	        		int keyAction = event.getAction();
	        		int keyCode = event.getKeyCode();
	        		if (keyAction == KeyEvent.ACTION_DOWN) {
						if (keyCode == KeyEvent.KEYCODE_BACK) {
							v.clearFocus();
							if ((flags & CONSOLE_ACTIVE) != 0 || (flags & CONSOLE_ACTIVE_FULLSCREEN) != 0) {
	    						quickCommand("toggleconsole");
	    						return true;
							} else if ((flags & CHAT_SAY) != 0
									|| (flags & UI_EDITINGFIELD) != 0
									|| (flags & CHAT_SAY_TEAM) != 0) {
								keypress(1, JAKeycodes.A_ESCAPE.ordinal(), 0);
								keypress(0, JAKeycodes.A_ESCAPE.ordinal(), 0);
	    						return true;
							}
						}
						keypress(1, MobileKeycodes.getMobileKey(keyCode, uc), uc);
						keypress(0, MobileKeycodes.getMobileKey(keyCode, uc), uc);
						return true;
					}
	        		return false;
	            }
	        });
		}
		@Override
		public boolean onKeyPreIme(int keyCode, KeyEvent event) {
			if (event.getAction() == KeyEvent.ACTION_DOWN) {
				if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
					if ((flags & CONSOLE_ACTIVE) != 0) {
						quickCommand("toggleconsole");
					} else if ((flags & CHAT_SAY) != 0
							|| (flags & UI_EDITINGFIELD) != 0
							|| (flags & CHAT_SAY_TEAM) != 0) {
						keypress(1, JAKeycodes.A_ESCAPE.ordinal(), 0);
						keypress(0, JAKeycodes.A_ESCAPE.ordinal(), 0);
					}
				}
			}
			return super.dispatchKeyEvent(event);
		}
	}
	private class jaMMESeekBar extends SeekBar {
		String LOG = "jaMME SeekBar";
    	public jaMMESeekBar(Context context) {
			super(context);
			float d = density;
			setMax(demoGetLength());
			setProgress(demoGetTime());
			setLeft(10);
			setY(surfaceHeight - 42*d);
			this.setPadding((int)(50*d), 0, (int)(50*d), 0);
			this.setThumbOffset((int)(50*d));
			setProgressDrawable(getResources().getDrawable(R.drawable.jamme_progress));
			setThumb(getResources().getDrawable(R.drawable.jamme_control));
			ViewGroup.LayoutParams lp = new ViewGroup.LayoutParams
					(ViewGroup.LayoutParams.MATCH_PARENT,
					ViewGroup.LayoutParams.WRAP_CONTENT);
			setLayoutParams(lp);
			setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
				@Override
				public void onProgressChanged(SeekBar seekBar, int progressValue, boolean fromUser) {
				  	Log.d(LOG, "onProgressChanged");
		            if (fromUser)
		            	quickCommand("seek "+(progressValue/1000.0));
				}
				@Override
				public void onStartTrackingTouch(SeekBar seekBar) {
				}
				@Override
				public void onStopTrackingTouch(SeekBar seekBar) {
				}
			});
		}
	}
	private Integer lastMin = 0, lastMax = 0;
	private class jaMMERangeSeekBar extends RangeSeekBar<Integer> {
		String LOG = "jaMME RangeSeekBar";
		short lastChange = 0;
		public jaMMERangeSeekBar(Integer min, Integer max, Context context) {
			super(min, max, (int)(50*density), context);
			float d = density;
//			setMax(demoGetLength());
			lastMin = 0;
			lastMax = this.getAbsoluteMaxValue();
//			setProgress(demoGetTime());
			setNotifyWhileDragging(true);
			setLeft(10);
			setY(surfaceHeight - 87*d);
			setPadding((int)(50*d), 0, (int)(50*d), 0);
//			this.setThumbOffset((int)(50*d));
//			setProgressDrawable(getResources().getDrawable(R.drawable.jamme_progress));
//			setThumb(getResources().getDrawable(R.drawable.jamme_control));
			ViewGroup.LayoutParams lp = new ViewGroup.LayoutParams
					(ViewGroup.LayoutParams.MATCH_PARENT,
					ViewGroup.LayoutParams.WRAP_CONTENT);
			setLayoutParams(lp);
			setOnRangeSeekBarChangeListener(new OnRangeSeekBarChangeListener<Integer>() {
				@SuppressLint("DefaultLocale")
				@Override
				public void onRangeSeekBarValuesChanged(RangeSeekBar<?> bar, Integer minValue, Integer maxValue) {
				  	Log.d(LOG, "onRangeSeekBarValuesChanged");
					if ((int)lastMin != (int)minValue) {
						quickCommand(String.format("seek %d.%03d", (minValue/1000), (minValue%1000)));
						lastMin = minValue;
						lastChange = -1;
					} else if ((int)lastMax != (int)maxValue) {
						quickCommand(String.format("seek %d.%03d", (maxValue/1000), (maxValue%1000)));
						lastMax = maxValue;
						lastChange = 1;
					}
				}
			});
		}
		@SuppressLint({ "ClickableViewAccessibility", "DefaultLocale" })
		@Override
		public boolean onTouchEvent(MotionEvent event) {
			int action = event.getAction();
			int actionCode = action & MotionEvent.ACTION_MASK;
			if (actionCode == MotionEvent.ACTION_UP) {
				if (lastChange < 0) {
					quickCommand(String.format("cut start %d.%03d", (lastMin/1000), (lastMin%1000)));
				} else if (lastChange > 0) {
					quickCommand(String.format("cut end %d.%03d", (lastMax/1000), (lastMax%1000)));
				}
				lastChange = 0;
//				return true;
			}
			return super.onTouchEvent(event);
		}
	}
	private class jaMMEView extends GLSurfaceView {
    	static final String LOG = "jaMME View";
    	@SuppressLint("ClickableViewAccessibility")
		public jaMMEView(Context context) {
    		super(context);
            setFocusableInTouchMode(true);
    	}
        @Override
        public boolean onCheckIsTextEditor() {
            Log.d(LOG, "onCheckIsTextEditor");
            return true;
        }
        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            Log.d(LOG, "onCreateInputConnection");
            BaseInputConnection fic = new BaseInputConnection(this, true);
            outAttrs.actionLabel = null;
            outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
            outAttrs.imeOptions = (EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_FLAG_NO_FULLSCREEN);
            return fic;
        }
    }
	private static boolean isLatinLetter(char c) {
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	}
	private static boolean isDigit(char c) {
		return (c >= '0' && c <= '9');
	}
	private class jaMMETextWatcher implements TextWatcher {
    	static final String LOG = "jaMME TextWatcher";
		@Override
		public void beforeTextChanged(CharSequence s, int start, int count, int after) {
			Log.d(LOG, "beforeTextChanged");
		}
		@Override
		public void onTextChanged(CharSequence s, int start, int before, int count) {
			Log.d(LOG, "onTextChanged");
		}
		@Override
		public void afterTextChanged(Editable s) {
			Log.d(LOG, "afterTextChanged");
			String ss = s.toString();
			if (ignoreAfter) {
				ignoreAfter = false;
				return;
			}
			if (ss == null)
				return;
			int l = ss.length();
			if (l < prevLength && l >= 0) {
//				int d = prevLength - l;
//				for (int i = d; i > 0; i--) {
					keypress(1, JAKeycodes.A_BACKSPACE.ordinal(), 0);
					keypress(0, JAKeycodes.A_BACKSPACE.ordinal(), 0);
//				}
				prevLength = 0;
				s.replace(0, s.length(), "");
			} else if (l > 0) {
				int d = l - prevLength;
				for (int i = d; i > 0; i--) {
					char ch = ss.toCharArray()[l-i];
					keypress(1, MobileKeycodes.getMobileKey(0, ch), ch);
					keypress(0, MobileKeycodes.getMobileKey(0, ch), ch);
				}
				prevLength = l;
			}
		}
    }
	ProgressDialog pd = null;
	String loadingMsg = "Loading...", lmLast = "Loading...";
	public static int LOADING_MESSAGE_TIME = 2000;
	public static int LOADING_MESSAGE_UPDATE_TIME = 1337;
	Runnable _loadingMessage = new Runnable() { 
	    public void run() {
	    	loadingMsg = getLoadingMsg();
			act.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					if (!jaMME.equals(loadingMsg, lmLast)) {
						lmLast = loadingMsg;
						if (pd != null)
							pd.dismiss();
						pd = null;
					}
					if (pd == null && !jaMME.equals(loadingMsg, "")) {
						pd = ProgressDialog.show(act, "", loadingMsg, true);
					}
				}
			});
	        _handler.postDelayed(_loadingMessage, LOADING_MESSAGE_UPDATE_TIME);
	    }   
	};
	private boolean logging = false;
	private void saveLog() {
		if (!logging)
			return;
        try {
            String[] command = new String[] {"logcat", "-v", "threadtime", "-f", gamePath+"/jamme.log"};
            Process process = Runtime.getRuntime().exec(command);
/*            BufferedReader bufferedReader = 
              new BufferedReader(new InputStreamReader(process.getInputStream()));

            String line;
            while ((line = bufferedReader.readLine()) != null){
                if(line.contains(processId)) {
                    LogFileWriter.write(line);
                }
            }*/
        } catch (IOException ex) {
            Log.e("jaMME saveLog", "getCurrentProcessLog failed", ex);
        }
    }
	private void removeView(View v) {
		if (v != null) {
			ViewGroup vg = (ViewGroup)v.getParent();
			if (vg != null)
				vg.removeView(v);
			v = null;
		}
	}
	private class jaMMERenderer implements GLSurfaceView.Renderer {
		static final String LOG = "jaMME Renderer";
		@Override
		public void onSurfaceCreated(GL10 gl, EGLConfig config) {
			Log.d(LOG, "onSurfaceCreated");
		}
		private void initRenderer(int width, int height){
			saveLog();
			Log.i(LOG, "screen size : " + width + "x"+ height);		
			setScreenSize(width,height);
			Log.i(LOG, "Init start");
			//TODO: filter out fs_basepath from gameArgs 
			String[] args_array = createArgs("+set fs_basepath \""+gamePath+"\" "+gameArgs);
			//entTODO: need to test on different devices
			String abi = Build.CPU_ABI, abi_alt = Build.CPU_ABI2;
			init(act,args_array,gamePath,getApplicationInfo().nativeLibraryDir,act.getFilesDir().toString(),abi,abi_alt);
			Log.i(LOG, "Init done");
		}
		@Override
		public void onSurfaceChanged(GL10 gl, int width, int height) {
			//surface changed called on surface created so.....
			Log.d(LOG, String.format("onSurfaceChanged %dx%d", width,height) );
			surfaceWidth = width;
			surfaceHeight = height;
		}
		int notifiedflags = 0;
		boolean inited = false;
		@Override
		public void onDrawFrame(GL10 gl) {
			if (!gameReady)
				return;
	        _handler.postDelayed(_loadingMessage, LOADING_MESSAGE_TIME);
			if (!inited) {
				SDLActivity.nativeInit();
				inited = true;
				initRenderer(surfaceWidth, surfaceHeight);
			}
			flags = frame();
	        _handler.removeCallbacks(_loadingMessage);
			act.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					if (pd != null)
						pd.dismiss();
					pd = null;
				}
		    });
//			flags |= (demoCutting) ? (DEMO_CUTTING) : (0);
			if (flags != notifiedflags) {
				if (((flags ^ notifiedflags) & CONSOLE_ACTIVE) != 0
						|| ((flags ^ notifiedflags) & UI_EDITINGFIELD) != 0
						|| ((flags ^ notifiedflags) & CHAT_SAY) != 0
						|| ((flags ^ notifiedflags) & CHAT_SAY_TEAM) != 0) {
					Log.d(LOG,"keyboard");
					final int fl = flags;
					Runnable r = new Runnable() { //doing this on the ui thread because android sucks.
						public void run() {
							InputMethodManager im = (InputMethodManager)act.getSystemService(Context.INPUT_METHOD_SERVICE);
							if (im != null) {
								if ((!keyboardActive && (fl & CONSOLE_ACTIVE) != 0
										|| (fl & UI_EDITINGFIELD) != 0
										|| (fl & CHAT_SAY) != 0
										|| (fl & CHAT_SAY_TEAM) != 0)) {
									removeView(et);
									et = new jaMMEEditText(act);
							        addContentView(et, et.getLayoutParams());
							        et.requestFocus();
									et.requestFocusFromTouch();
									//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
									im.showSoftInput(et, 0);//InputMethodManager.SHOW_FORCED);
									keyboardActive = true;
								} else {
									//getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
									im.hideSoftInputFromWindow(view.getWindowToken(), 0);
									keyboardActive = false;
								}
							} else {
								Log.e(LOG, "IMM failed");
							}
						}	
					};
					act.runOnUiThread(r);
				} else if ((((flags ^ notifiedflags) & DEMO_PAUSED) != 0)
//						|| ((flags ^ notifiedflags) & DEMO_CUTTING) != 0
						) {
					Log.d(LOG,"seekbar");
					final int fl = flags;
					Runnable r = new Runnable() { //doing this on the ui thread because android sucks.
						public void run() {
//always draw demo cut inteface
							if ((fl & DEMO_PAUSED) != 0) {
						        if (sb == null) {
									sb = new jaMMESeekBar(act);
									addContentView(sb, sb.getLayoutParams());
						        } else {
						        	sb.setVisibility(View.VISIBLE);
						        }
								if (rsb == null) {
							        rsb = new jaMMERangeSeekBar(0, demoGetLength(), act);
									addContentView(rsb, rsb.getLayoutParams());
								} else {
						        	rsb.setVisibility(View.VISIBLE);
						        }
							} else {
								if (sb != null) {
						        	sb.setVisibility(View.INVISIBLE);
						        }
								if (rsb != null) {
						        	rsb.setVisibility(View.INVISIBLE);
						        }
							}
						}	
					};
					act.runOnUiThread(r);
				} else if (((flags ^ notifiedflags) & DEMO_PLAYBACK) != 0) {
					Log.d(LOG,"seekbar reset");
					Runnable r = new Runnable() { //doing this on the ui thread because android sucks.
						public void run() {
							if (sb != null) {
								ViewGroup vg = (ViewGroup)sb.getParent();
								if (vg != null)
									vg.removeView(sb);
								sb = null;
							}
							if (rsb != null) {
								ViewGroup vg = (ViewGroup)rsb.getParent();
								if (vg != null)
									vg.removeView(rsb);
								rsb = null;
							}
						}	
					};
					act.runOnUiThread(r);
//					}
				}
				notifiedflags = flags;
			}
			if ((flags & DEMO_PAUSED) != 0 && sb != null)
				sb.setProgress(demoGetTime());
			vibrate();
		}   	
    }
	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) { 	   
		int uc = 0;
		if (event !=null)
			uc = event.getUnicodeChar();
		if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
			quickCommand("chase targetNext");
		} else if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
			quickCommand("chase targetPrev");
		} else/* if (keyCode == KeyEvent.KEYCODE_ESCAPE
				   || keyCode == KeyEvent.KEYCODE_BACK
				   || keyCode == KeyEvent.KEYCODE_MENU
				   || keyCode == KeyEvent.KEYCODE_DEL)*/ {
			if (((flags & CONSOLE_ACTIVE) != 0 || (flags & CONSOLE_ACTIVE_FULLSCREEN) != 0)
					&& keyCode == KeyEvent.KEYCODE_BACK) {
				quickCommand("toggleconsole");
				return true;
			} else if ((flags & CONSOLE_ACTIVE) != 0 && keyCode == KeyEvent.KEYCODE_MENU) {
				quickCommand("toggleconsole");
				return true;
			}
			keypress(1, MobileKeycodes.getMobileKey(keyCode, uc), uc);
			keypress(0, MobileKeycodes.getMobileKey(keyCode, uc), uc);
			if (keyCode == KeyEvent.KEYCODE_DEL && et != null) {
				et.setText("");
			}
		}/* else {
			return super.onKeyDown(keyCode, event); 
		}*/
		return true;
	}
	private String PasteFromClipboard() {
		ClipboardManager clipboard = (ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
		String paste = null;
		if (clipboard != null
				&& clipboard.hasPrimaryClip()
				&& clipboard.getPrimaryClipDescription() != null
				&& clipboard.getPrimaryClipDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN)
				&& clipboard.getPrimaryClip() != null
				&& clipboard.getPrimaryClip().getItemAt(0) != null) {
		    ClipData.Item item = clipboard.getPrimaryClip().getItemAt(0);
		    if (item.getText() != null) {
		    	paste = item.getText().toString();
		    }
		}
		return paste;
	}
	private static int LONG_PRESS_TIME = 717;
	private final Handler _handler = new Handler(); 
	Runnable _longPressed = new Runnable() { 
	    public void run() {
	    	if (twoPointers) {
	    		quickCommand("screenshot");
	    	} else if ((flags & DEMO_PLAYBACK) != 0) {
	    		quickCommand("cut it");
	    	} else if (!textPasted && ((flags & CONSOLE_ACTIVE) != 0
	    			|| (flags & UI_EDITINGFIELD) != 0
	    			|| (flags & CHAT_SAY) != 0
	    			|| (flags & CHAT_SAY_TEAM) != 0)) {
				String paste = PasteFromClipboard();
				if (paste != null) {
					try {
						textPaste(paste.getBytes("US-ASCII"));
						textPasted = true;
					} catch (UnsupportedEncodingException e) {
						e.printStackTrace();
					}
				}
/*			} else if ((flags & DEMO_PAUSED) != 0) {
				if (demoCutting) {
					demoCutting = false;
				} else {
					demoCutting = true;
				}*/
			} else if (flags == 0) {
	    		quickCommand("+scores");
			}
	    	longPress = true;
	    }   
	};
	private float touchX = 0, touchY = 0;
	private float touchYtotal = 0;
	private boolean touchMotion = false, longPress = false, textPasted = false, twoPointers = false;
//	@Override
	public boolean onTouchEvent(MotionEvent event) {
    	int action = event.getAction();
		int actionCode = action & MotionEvent.ACTION_MASK;
		int pointerCount = event.getPointerCount();
		if (pointerCount == 2) {
			twoPointers = true;
		}
		if (actionCode == MotionEvent.ACTION_DOWN) {
			touchX = event.getX();
			touchY = event.getY();
	        _handler.postDelayed(_longPressed, LONG_PRESS_TIME);
		} else if (actionCode == MotionEvent.ACTION_MOVE && !twoPointers) {
			float d = density;
			float deltaX = (event.getX() - touchX) / surfaceWidth;
			float deltaY = (event.getY() - touchY) / surfaceHeight;
			float adx = Math.abs(deltaX*1.7f*d);
			float ady = Math.abs(deltaY*1.7f*d);
			adx *= 300;
			ady *= 300;
			if (!touchMotion && longPress) {
				return true;
			} else if ((!touchMotion && (adx >= 4.5f || ady >= 4.5f))) {
				touchMotion = true;
		        _handler.removeCallbacks(_longPressed);
			} else if (!touchMotion) {
				return true;
			}
			if ((flags & CONSOLE_ACTIVE) == 0 && (flags & CONSOLE_ACTIVE_FULLSCREEN) == 0) {
				mouseMove(deltaX*1.7f*d, deltaY*1.7f*d);
			} else {
				touchYtotal += deltaY*23*d*2;
				int tYt = (int)Math.abs(touchYtotal);
				if (tYt > 1) {
					int qkey = 0;
					//scroll the console
					if ((flags & CONSOLE_ACTIVE_FULLSCREEN) != 0) {
						if (deltaY < 0) {
							qkey = JAKeycodes.A_MWHEELDOWN.ordinal();
						} else if (deltaY > 0) {
							qkey = JAKeycodes.A_MWHEELUP.ordinal();
						}
					//scroll the console input history
					} else if ((flags & CONSOLE_ACTIVE) != 0) {
						if (deltaY < 0) {
							qkey = JAKeycodes.A_CURSOR_DOWN.ordinal();
						} else if (deltaY > 0) {
							qkey = JAKeycodes.A_CURSOR_UP.ordinal();
						}
						tYt = (int)(tYt / 1.7);
					}
					for (int i = 0; i < tYt; i++) {
						keypress(1, qkey, 0);
						keypress(0, qkey, 0);
					}
					touchYtotal -= (int)touchYtotal;
				}
			}
			touchX = event.getX();
			touchY = event.getY();
		} else if (actionCode == MotionEvent.ACTION_POINTER_UP && twoPointers) {
			if (!longPress) {
				quickCommand("toggleconsole");
			}
			touchMotion = true;
    	} else if (actionCode == MotionEvent.ACTION_UP) {
        	if (!touchMotion) {
				if ((flags & ~INGAME) == 0 && !longPress) {
					if ((flags & CHAT_SAY) == 0 && (flags & CHAT_SAY_TEAM) == 0) {
						quickCommand("messagemode");
					} else if ((flags & CHAT_SAY) != 0) {
						//hack: the 1st call resets chat, and the 2nd call actually calls team chat
						quickCommand("messagemode2; messagemode2");
					} else if ((flags & CHAT_SAY_TEAM) != 0) {
						keypress(1, JAKeycodes.A_ESCAPE.ordinal(), 0);
						keypress(0, JAKeycodes.A_ESCAPE.ordinal(), 0);
					}
				} else if ((flags & CONSOLE_ACTIVE) == 0 && (flags & CONSOLE_ACTIVE_FULLSCREEN) == 0) {
					if ((flags & DEMO_PLAYBACK) == 0) {
						if (!longPress) {
							keypress(1, JAKeycodes.A_MOUSE1.ordinal(), 0);
							keypress(0, JAKeycodes.A_MOUSE1.ordinal(), 0);
						}
					} else {
						quickCommand("pause");
					}
				}
			}
			if (longPress && !twoPointers)
				quickCommand("-scores");
			touchMotion = false;
			longPress = false;
			textPasted = false;
			twoPointers = false;
			touchYtotal = 0;
	        _handler.removeCallbacks(_longPressed);
		}
		return true;
    }
    private String[] createArgs(String appArgs) {
		ArrayList<String> a = new ArrayList<String>(Arrays.asList(appArgs.split(" ")));
		Iterator<String> iter = a.iterator();
		while (iter.hasNext()) {
			if (iter.next().contentEquals("")) {
				iter.remove();
			}
		}
		return a.toArray(new String[a.size()]);
	}
	private static final String []demoExtList = {
		".dm_26",
		".dm_25",
		".mme",
		null
	};
}

class BestEglChooser implements EGLConfigChooser {
	static final String LOG = "BestEglChooser";

	Context ctx;

	public BestEglChooser(Context ctx)
	{
		this.ctx = ctx;
	}


	public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {

		Log.i( LOG, "chooseConfig");


		int[][] mConfigSpec  = {	
				{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 24,
					EGL10.EGL_STENCIL_SIZE, 8,
					EGL10.EGL_NONE
				},{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 24,
					EGL10.EGL_STENCIL_SIZE, 0,
					EGL10.EGL_NONE
				},
				{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 16,
					EGL10.EGL_STENCIL_SIZE, 8,
					EGL10.EGL_NONE
				},{
					EGL10.EGL_RED_SIZE, 8,
					EGL10.EGL_GREEN_SIZE, 8,
					EGL10.EGL_BLUE_SIZE, 8,
					EGL10.EGL_ALPHA_SIZE, 8,
					EGL10.EGL_DEPTH_SIZE, 16,
					EGL10.EGL_STENCIL_SIZE, 0,
					EGL10.EGL_NONE
				},
		};

		Log.i(LOG,"Number of specs to test: " + mConfigSpec.length);

		int specChosen;
		int[] num_config = new int[1];
		int numConfigs=0;
		for (specChosen=0;specChosen<mConfigSpec.length;specChosen++)
		{
			egl.eglChooseConfig(display, mConfigSpec[specChosen], null, 0, num_config);
			if ( num_config[0] >0)
			{
				numConfigs =  num_config[0];
				break;
			}
		}

		if (specChosen ==mConfigSpec.length) {
			throw new IllegalArgumentException(
					"No EGL configs match configSpec");
		}

		EGLConfig[] configs = new EGLConfig[numConfigs];
		egl.eglChooseConfig(display, mConfigSpec[specChosen], configs, numConfigs,	num_config);

		String eglConfigsString = "";
		for(int n=0;n<configs.length;n++) {
			Log.i( LOG, "found EGL config : " + printConfig(egl,display,configs[n])); 
			eglConfigsString += n + ": " +  printConfig(egl,display,configs[n]) + ",";
		}
		Log.i(LOG,eglConfigsString);
//		AppSettings.setStringOption(ctx, "egl_configs", eglConfigsString);

			
		int selected = 0;
		
		int override = 0;//AppSettings.getIntOption(ctx, "egl_config_selected", 0);
		if (override < configs.length)
		{
			selected = override;
		}
			
		// best choice : select first config
		Log.i( LOG, "selected EGL config[" + selected + "]: " + printConfig(egl,display,configs[selected]));

		return configs[selected];
	}


	private  String printConfig(EGL10 egl, EGLDisplay display,
			EGLConfig config) {

		int r = findConfigAttrib(egl, display, config,
				EGL10.EGL_RED_SIZE, 0);
		int g = findConfigAttrib(egl, display, config,
				EGL10.EGL_GREEN_SIZE, 0);
		int b = findConfigAttrib(egl, display, config,
				EGL10.EGL_BLUE_SIZE, 0);
		int a = findConfigAttrib(egl, display, config,
				EGL10.EGL_ALPHA_SIZE, 0);
		int d = findConfigAttrib(egl, display, config,
				EGL10.EGL_DEPTH_SIZE, 0);
		int s = findConfigAttrib(egl, display, config,
				EGL10.EGL_STENCIL_SIZE, 0);

		/*
		 * 
		 * EGL_CONFIG_CAVEAT value 

     #define EGL_NONE		       0x3038	
     #define EGL_SLOW_CONFIG		       0x3050	
     #define EGL_NON_CONFORMANT_CONFIG      0x3051	
		 */

		return String.format("rgba=%d%d%d%d z=%d sten=%d", r,g,b,a,d,s)
				+ " n=" + findConfigAttrib(egl, display, config, EGL10.EGL_NATIVE_RENDERABLE, 0)
				+ " b=" + findConfigAttrib(egl, display, config, EGL10.EGL_BUFFER_SIZE, 0)
				+ String.format(" c=0x%04x" , findConfigAttrib(egl, display, config, EGL10.EGL_CONFIG_CAVEAT, 0))
				;

	}

	private int findConfigAttrib(EGL10 egl, EGLDisplay display,
			EGLConfig config, int attribute, int defaultValue) {

		int[] mValue = new int[1];
		if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
			return mValue[0];
		}
		return defaultValue;
	}

}
