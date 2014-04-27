package net.sourceforge.gigalomania;

import org.libsdl.app.SDLActivity; 

import android.content.Intent;
import android.net.Uri;
import android.util.Log;

/* 
 * A sample wrapper class that just calls SDLActivity 
 */ 

public class Gigalomania extends SDLActivity {
    private static final String TAG = "Gigalomania";

	public void launchUrl(String url) {
        Log.d(TAG, "launchUrl(): " + url);
        Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
    	startActivity(browserIntent);
	}
}
