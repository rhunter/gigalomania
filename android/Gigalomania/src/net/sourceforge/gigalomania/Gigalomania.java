package net.sourceforge.gigalomania;

import org.libsdl.app.SDLActivity; 

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

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

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        
        // keep screen active - see http://stackoverflow.com/questions/2131948/force-screen-on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
}
