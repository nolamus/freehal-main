package net.freehal.app;

import net.freehal.app.impl.FreehalUser;
import net.freehal.app.select.SelectContent;
import net.freehal.app.util.SpeechHelper;
import net.freehal.app.util.Util;
import net.freehal.app.util.VoiceRecHelper;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import com.actionbarsherlock.app.SherlockFragmentActivity;
import com.actionbarsherlock.view.Menu;
import com.actionbarsherlock.view.MenuInflater;
import com.actionbarsherlock.view.MenuItem;
import com.actionbarsherlock.view.MenuItem.OnMenuItemClickListener;

public class OverviewActivity extends SherlockFragmentActivity implements
		OverviewFragment.Callbacks {

	private static boolean mTwoPane;
	private String selectedTab;
	private OverviewFragment fragment;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_list);

		Util.setActivity(this);
		FreehalUser.init(this.getApplicationContext());
		SpeechHelper.getInstance().start();

		if (findViewById(R.id.detail_container) != null) {
			mTwoPane = true;
			fragment = (OverviewFragment) getSupportFragmentManager()
					.findFragmentById(R.id.list);
			fragment.setTwoPane(true);
			fragment.setActivateOnItemClick(true);

		}
		SelectContent.init(this.getResources());

		Intent intent = getIntent();
		String action = intent.getAction();
		String type = intent.getType();

		if (Intent.ACTION_SEND.equals(action) && type != null) {
			if ("text/plain".equals(type)) {
				recieveText(intent);
			}
		} else if (intent.getBooleanExtra("BY_SERVICE", false) == true) {
			startedByService(intent);
		} else {
			launch(savedInstanceState);
		}

		this.startService(new Intent(this, FreehalService.class));
	}

	public void launch(Bundle savedInstanceState) {
		if (mTwoPane) {
			DetailFragment.setRecievedIntent(null);

			final String tab;
			if (savedInstanceState != null
					&& savedInstanceState.containsKey("tab"))
				tab = savedInstanceState.getString("tab");
			else
				tab = "about";
			onItemSelected(tab);
		}
	}

	private void recieveText(Intent intent) {
		DetailFragment.setRecievedIntent(intent);
		onItemSelected("online");
	}

	private void startedByService(Intent intent) {
		onItemSelected("online");
	}

	@Override
	public void onItemSelected(String id) {

		if (selectedTab == "settings") {
			onItemSelected(selectedTab);
			Intent detailIntent = new Intent(this, FreehalPreferences.class);
			startActivity(detailIntent);

		} else if (mTwoPane) {
			selectedTab = id;
			DetailFragment fragment = DetailFragment.forTab(id);
			getSupportFragmentManager().beginTransaction()
					.replace(R.id.detail_container, fragment).commit();

		} else {
			selectedTab = id;
			Intent detailIntent = new Intent(this, DetailActivity.class);
			detailIntent.putExtra(DetailFragment.ARG_ITEM_ID, id);
			startActivity(detailIntent);
		}
	}

	@Override
	public void onSaveInstanceState(Bundle savedInstanceState) {
		super.onSaveInstanceState(savedInstanceState);
		if (selectedTab != null && selectedTab.length() > 0)
			savedInstanceState.putString("tab", selectedTab);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		doCreateOptionsMenu(menu, this, this.getApplicationContext());
		return true;
	}

	public static void doCreateOptionsMenu(Menu menu,
			final SherlockFragmentActivity activity, final Context appContext) {
		MenuInflater inflater = activity.getSupportMenuInflater();
		inflater.inflate(R.menu.actionbar, menu);

		// voice recognition
		if (VoiceRecHelper.hasVoiceRecognition(appContext)) {
			final MenuItem voiceRec = menu.findItem(R.id.menu_speak);
			if (mTwoPane)
				voiceRec.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS
						| MenuItem.SHOW_AS_ACTION_WITH_TEXT);
			voiceRec.setOnMenuItemClickListener(new OnMenuItemClickListener() {
				@Override
				public boolean onMenuItemClick(MenuItem item) {
					VoiceRecHelper.start(activity, appContext);
					return true;
				}
			});
		} else {
			menu.removeItem(R.id.menu_speak);
		}

		// preferences
		{
			final MenuItem prefs = menu.findItem(R.id.menu_settings);
			if (mTwoPane)
				prefs.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS
						| MenuItem.SHOW_AS_ACTION_WITH_TEXT);
			prefs.setOnMenuItemClickListener(new FreehalPreferences.Listener(
					activity));
		}
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == VoiceRecHelper.REQUEST_CODE) {
			VoiceRecHelper.onActivityResult(requestCode, resultCode, data);
		}
		super.onActivityResult(requestCode, resultCode, data);
	}
}