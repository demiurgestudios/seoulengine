/**
 * \file EditTextBackEvent.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.PersistableBundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.SpannableStringBuilder;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import java.lang.Runnable;

@SuppressLint("AppCompatCustomView")
public class EditTextBackEvent extends EditText {

	private EditTextImeBackListener mOnImeBack;

	public EditTextBackEvent(Context context) {
		super(context);
	}

	public EditTextBackEvent(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public EditTextBackEvent(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}

	@Override
	public boolean onKeyPreIme(int keyCode, KeyEvent event) {
		if (event.getKeyCode() == KeyEvent.KEYCODE_BACK &&
			event.getAction() == KeyEvent.ACTION_UP) {
			if (mOnImeBack != null)
				mOnImeBack.onImeBack(this, this.getText().toString());
		}
		return super.dispatchKeyEvent(event);
	}

	public void setOnEditTextImeBackListener(EditTextImeBackListener listener) {
		mOnImeBack = listener;
	}
}
