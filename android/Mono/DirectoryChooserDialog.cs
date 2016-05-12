using System;
using System.Collections.Generic;

using Android.App;
using Android.Content;
using Android.Graphics;
using Android.OS;
using Android.Text;
using Android.Util;
using Android.Views;
using Android.Widget;

using Java.IO;

namespace android {
	public class DirectoryChooserDialog {
		private bool m_isNewFolderEnabled = false;//true;
		private string m_sdcardDirectory = "";
		private Context m_context;
		private TextView m_titleView;

		private string m_dir = "";
		private List<string> m_subdirs = null;
		private ChosenDirectoryListener m_chosenDirectoryListener = null;
		private ArrayAdapterOverrided m_listAdapter = null;

		//////////////////////////////////////////////////////
		// Callback interface for selected directory
		//////////////////////////////////////////////////////
		public interface ChosenDirectoryListener {
			void onChosenDir(string chosenDir);
		}

		public DirectoryChooserDialog(Context context, ChosenDirectoryListener chosenDirectoryListener) {
			m_context = context;
			m_sdcardDirectory = Android.OS.Environment.ExternalStorageDirectory.AbsolutePath;
			m_chosenDirectoryListener = chosenDirectoryListener;

			try {
				m_sdcardDirectory = new File(m_sdcardDirectory).CanonicalPath;
			} catch (IOException exception) {
				exception.PrintStackTrace();
			}
		}

		///////////////////////////////////////////////////////////////////////
		// setNewFolderEnabled() - enable/disable new folder button
		///////////////////////////////////////////////////////////////////////

		public void setNewFolderEnabled(bool isNewFolderEnabled) {
			m_isNewFolderEnabled = isNewFolderEnabled;
		}

		public bool getNewFolderEnabled() {
			return m_isNewFolderEnabled;
		}

		///////////////////////////////////////////////////////////////////////
		// chooseDirectory() - load directory chooser dialog for initial
		// default sdcard directory
		///////////////////////////////////////////////////////////////////////

		public void chooseDirectory() {
			// Initial directory is sdcard directory
			chooseDirectory(m_sdcardDirectory);
		}

		class DirectoryOnClickListener : Java.Lang.Object, IDialogInterfaceOnClickListener {
			private DirectoryChooserDialog dirChooser;

			private DirectoryOnClickListener() { }
			public DirectoryOnClickListener(DirectoryChooserDialog dirChooser) {
				this.dirChooser = dirChooser;
			}

			public void OnClick(IDialogInterface dialog, int item) {
				// Navigate into the sub-directory
				this.dirChooser.m_dir += "/" + ((AlertDialog)dialog).ListView.Adapter.GetItem(item);

				try {
					this.dirChooser.m_dir =  new File(this.dirChooser.m_dir).CanonicalPath;
				} catch (IOException exception) {
					// TODO Auto-generated catch block
					exception.PrintStackTrace();
				}

				this.dirChooser.updateDirectory();
			}
		}
		////////////////////////////////////////////////////////////////////////////////
		// chooseDirectory(String dir) - load directory chooser dialog for initial 
		// input 'dir' directory
		////////////////////////////////////////////////////////////////////////////////

		public void chooseDirectory(string dir) {
			File dirFile = new File(dir);

			Log.Debug("dir", "dir = " + dir);

			if (!dirFile.Exists() || !dirFile.IsDirectory) {
				dir = m_sdcardDirectory;
			}

			try {
				dir = new File(dir).CanonicalPath;
			} catch (IOException ioe) {
				return;
			}

			m_dir = dir;

			m_subdirs = getDirectories(dir);

			AlertDialog.Builder dialogBuilder =
				createDirectoryChooserDialog(dir, m_subdirs, new DirectoryOnClickListener(this));
			dialogBuilder.SetPositiveButton("OK", (sender, ev) => {
				// Current directory chosen
				if (m_chosenDirectoryListener != null) {
					// Call registered listener supplied with the chosen directory
					m_chosenDirectoryListener.onChosenDir(m_dir);
				}
			}).SetNegativeButton("Cancel", (sender, ev) => { });
			AlertDialog dirsDialog = dialogBuilder.Create();
			Button b = dirsDialog.GetButton((int)DialogButtonType.Negative);
			if (b != null) {
				b.Background = m_context.Resources.GetDrawable(Resource.Drawable.jamme_btn, m_context.Theme);
			}
			b = dirsDialog.GetButton((int)DialogButtonType.Neutral);
			if (b != null) {
				b.Background = m_context.Resources.GetDrawable(Resource.Drawable.jamme_btn, m_context.Theme);
			}
			b = dirsDialog.GetButton((int)DialogButtonType.Positive);
			if (b != null) {
				b.Background = m_context.Resources.GetDrawable(Resource.Drawable.jamme_btn, m_context.Theme);
			}
			dirsDialog.KeyPress += (sender, ev) => {
				if (ev.KeyCode == Keycode.Back && ev.Event.Action == KeyEventActions.Down) {
					// Back button pressed
					if (m_dir.Equals("/")) {
						// The very top level directory, do nothing
						ev.Handled = false;
						return;
					} else {
						// Navigate back to an upper directory
						m_dir = new File(m_dir).Parent;
						this.updateDirectory();
					}
					ev.Handled = true;
					return;
				} else {
					ev.Handled = false;
					return;
				}
			};
			// Show directory chooser dialog
			dirsDialog.Show();
		}
		private bool createSubDir(string newDir) {
			File newDirFile = new File(newDir);
			if (!newDirFile.Exists()) {
				return newDirFile.Mkdir();
			}
			return false;
		}
		private List<string> getDirectories(string dir) {
			var dirs = new List<string>();

			dirs.Add("..");

			try {
				File dirFile = new File(dir);
				if (!dirFile.Exists() || !dirFile.IsDirectory) {
					return dirs;
				}

				foreach (var file in dirFile.ListFiles()) {
					if (file.IsDirectory) {
						dirs.Add(file.Name);
					}
				}
			} catch (Java.Lang.Exception exception) {
				exception.PrintStackTrace();
			}

			dirs.Sort(delegate (string o1, string o2) {
				return o1.CompareTo(o2);
			});

			return dirs;
		}
		private AlertDialog.Builder createDirectoryChooserDialog(string title, List<string> listItems,
				IDialogInterfaceOnClickListener onClickListener) {
			AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(m_context);

			// Create custom view for AlertDialog title containing 
			// current directory TextView and possible 'New folder' button.
			// Current directory TextView allows long directory path to be wrapped to multiple lines.
			LinearLayout titleLayout = new LinearLayout(m_context);
			titleLayout.Orientation = Orientation.Vertical;

			m_titleView = new TextView(m_context);
			m_titleView.LayoutParameters = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MatchParent, ViewGroup.LayoutParams.WrapContent);
			m_titleView.SetTextAppearance(m_context, Android.Resource.Style.TextAppearanceLarge);
			m_titleView.SetTextColor(Color.White);
			m_titleView.Gravity = GravityFlags.CenterVertical | GravityFlags.CenterHorizontal;
			m_titleView.Text = title;

			Button newDirButton = new Button(m_context);
			newDirButton.LayoutParameters = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MatchParent, ViewGroup.LayoutParams.WrapContent);
			newDirButton.Text = "New folder";
			newDirButton.Click += (sender, ev) => {
				EditText input = new EditText(m_context);

				// Show new folder name input dialog
				new AlertDialog.Builder(m_context).
				SetTitle("New folder name").
				SetView(input).SetPositiveButton("OK", (sen, eve) => {
					string newDir = input.Text;
				// Create new directory
				if (createSubDir(m_dir + "/" + newDir)) {
					// Navigate into the new directory
					m_dir += "/" + newDir;
						this.updateDirectory();
					} else {
						Toast.MakeText(
							m_context, "Failed to create '" + newDir +
							"' folder", ToastLength.Short).Show();
					}
				}).SetNegativeButton("Cancel", (sen, eve) => { }).Show();
			};

			if (!m_isNewFolderEnabled) {
				newDirButton.Visibility = ViewStates.Gone;
			}

			titleLayout.AddView(m_titleView);
			titleLayout.AddView(newDirButton);

			dialogBuilder.SetCustomTitle(titleLayout);

			m_listAdapter = new ArrayAdapterOverrided(m_context, listItems);

			dialogBuilder.SetSingleChoiceItems(m_listAdapter, -1, onClickListener);
			dialogBuilder.SetCancelable(false);

			return dialogBuilder;
		}
		private void updateDirectory() {
/*			m_subdirs.Clear();
			m_subdirs.AddRange(getDirectories(m_dir));*/
			m_listAdapter.Clear();
			m_listAdapter.AddAll(getDirectories(m_dir));
			m_titleView.Text = m_dir;

			m_listAdapter.NotifyDataSetChanged();
		}

		private class ArrayAdapterOverrided : ArrayAdapter<string> {
			public ArrayAdapterOverrided(Context context, List<string> items) : base(context,
					Android.Resource.Layout.SelectDialogItem, Android.Resource.Id.Text1, items) {

			}
			public override View GetView(int position, View convertView,
					ViewGroup parent) {
				View v = base.GetView(position, convertView, parent);

				if (v is TextView) {
					// Enable list item (directory) text wrapping
					TextView tv = (TextView)v;
					tv.LayoutParameters.Height = ViewGroup.LayoutParams.WrapContent;
					tv.Ellipsize = null;
				}
				return v;
			}
		}
	}
}