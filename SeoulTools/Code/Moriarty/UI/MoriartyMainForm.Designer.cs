// Root windows form of the Moriarty application.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace Moriarty.UI
{
	partial class MoriartyMainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
			this.components = new System.ComponentModel.Container();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MoriartyMainForm));
			this.m_MenuStrip = new System.Windows.Forms.MenuStrip();
			this.m_FileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.m_SaveUserSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.m_FileMenuToolStripSeperator = new System.Windows.Forms.ToolStripSeparator();
			this.m_ExitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.m_ViewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.m_KeyboardCaptureWindowToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.m_HelpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.m_AboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.m_StatusStrip = new System.Windows.Forms.StatusStrip();
			this.m_ConnectionCountLabel = new System.Windows.Forms.ToolStripStatusLabel();
			this.m_Split = new System.Windows.Forms.SplitContainer();
			this.m_ConnectionsPreferencesSplitPanel = new System.Windows.Forms.SplitContainer();
			this.m_ConnectionList = new System.Windows.Forms.ListView();
			this.m_ConnectionListIcons = new System.Windows.Forms.ImageList(this.components);
			this.m_UserSettings = new System.Windows.Forms.PropertyGrid();
			this.m_MoriartyConnectionControl = new Moriarty.UI.MoriartyConnectionControl();
			this.m_MainFormToolStrip = new System.Windows.Forms.ToolStrip();
			this.m_TargetSelectionComboBox = new System.Windows.Forms.ToolStripComboBox();
			this.m_RunTargetButton = new System.Windows.Forms.ToolStripButton();
			this.m_MenuStrip.SuspendLayout();
			this.m_StatusStrip.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.m_Split)).BeginInit();
			this.m_Split.Panel1.SuspendLayout();
			this.m_Split.Panel2.SuspendLayout();
			this.m_Split.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.m_ConnectionsPreferencesSplitPanel)).BeginInit();
			this.m_ConnectionsPreferencesSplitPanel.Panel1.SuspendLayout();
			this.m_ConnectionsPreferencesSplitPanel.Panel2.SuspendLayout();
			this.m_ConnectionsPreferencesSplitPanel.SuspendLayout();
			this.m_MainFormToolStrip.SuspendLayout();
			this.SuspendLayout();
			//
			// m_MenuStrip
			//
			this.m_MenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.m_FileToolStripMenuItem,
            this.m_ViewToolStripMenuItem,
            this.m_HelpToolStripMenuItem});
			this.m_MenuStrip.Location = new System.Drawing.Point(0, 0);
			this.m_MenuStrip.Name = "m_MenuStrip";
			this.m_MenuStrip.Size = new System.Drawing.Size(792, 24);
			this.m_MenuStrip.TabIndex = 0;
			this.m_MenuStrip.Text = "m_MenuStrip";
			//
			// m_FileToolStripMenuItem
			//
			this.m_FileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.m_SaveUserSettingsToolStripMenuItem,
            this.m_FileMenuToolStripSeperator,
            this.m_ExitToolStripMenuItem});
			this.m_FileToolStripMenuItem.Name = "m_FileToolStripMenuItem";
			this.m_FileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
			this.m_FileToolStripMenuItem.Text = "&File";
			//
			// m_SaveUserSettingsToolStripMenuItem
			//
			this.m_SaveUserSettingsToolStripMenuItem.Name = "m_SaveUserSettingsToolStripMenuItem";
			this.m_SaveUserSettingsToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
			this.m_SaveUserSettingsToolStripMenuItem.Size = new System.Drawing.Size(203, 22);
			this.m_SaveUserSettingsToolStripMenuItem.Text = "&Save User Settings";
			this.m_SaveUserSettingsToolStripMenuItem.ToolTipText = "Save user settings (targets, launch options) to your local user settings file.";
			this.m_SaveUserSettingsToolStripMenuItem.Click += new System.EventHandler(this.m_SaveUserSettingsToolStripMenuItem_Click);
			//
			// m_FileMenuToolStripSeperator
			//
			this.m_FileMenuToolStripSeperator.Name = "m_FileMenuToolStripSeperator";
			this.m_FileMenuToolStripSeperator.Size = new System.Drawing.Size(200, 6);
			//
			// m_ExitToolStripMenuItem
			//
			this.m_ExitToolStripMenuItem.Name = "m_ExitToolStripMenuItem";
			this.m_ExitToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Alt | System.Windows.Forms.Keys.F4)));
			this.m_ExitToolStripMenuItem.Size = new System.Drawing.Size(203, 22);
			this.m_ExitToolStripMenuItem.Text = "E&xit";
			this.m_ExitToolStripMenuItem.ToolTipText = "Exit Moriarty, any active connections will be disconnected.";
			this.m_ExitToolStripMenuItem.Click += new System.EventHandler(this.m_ExitToolStripMenuItem_Click);
			//
			// m_ViewToolStripMenuItem
			//
			this.m_ViewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.m_KeyboardCaptureWindowToolStripMenuItem});
			this.m_ViewToolStripMenuItem.Name = "m_ViewToolStripMenuItem";
			this.m_ViewToolStripMenuItem.Size = new System.Drawing.Size(41, 20);
			this.m_ViewToolStripMenuItem.Text = "&View";
			//
			// m_KeyboardCaptureWindowToolStripMenuItem
			//
			this.m_KeyboardCaptureWindowToolStripMenuItem.Name = "m_KeyboardCaptureWindowToolStripMenuItem";
			this.m_KeyboardCaptureWindowToolStripMenuItem.Size = new System.Drawing.Size(203, 22);
			this.m_KeyboardCaptureWindowToolStripMenuItem.Text = "&Keyboard Capture Window";
			this.m_KeyboardCaptureWindowToolStripMenuItem.ToolTipText = "Display the keyboard capture window. Keyboard input will be sent to the selected " +
    "connection while this window has focus.";
			this.m_KeyboardCaptureWindowToolStripMenuItem.Click += new System.EventHandler(this.m_KeyboardCaptureWindowToolStripMenuItem_Click);
			//
			// m_HelpToolStripMenuItem
			//
			this.m_HelpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.m_AboutToolStripMenuItem});
			this.m_HelpToolStripMenuItem.Name = "m_HelpToolStripMenuItem";
			this.m_HelpToolStripMenuItem.Size = new System.Drawing.Size(40, 20);
			this.m_HelpToolStripMenuItem.Text = "&Help";
			//
			// m_AboutToolStripMenuItem
			//
			this.m_AboutToolStripMenuItem.Name = "m_AboutToolStripMenuItem";
			this.m_AboutToolStripMenuItem.Size = new System.Drawing.Size(146, 22);
			this.m_AboutToolStripMenuItem.Text = "&About Moriarty";
			this.m_AboutToolStripMenuItem.ToolTipText = "Be informed.";
			this.m_AboutToolStripMenuItem.Click += new System.EventHandler(this.m_AboutToolStripMenuItem_Click);
			//
			// m_StatusStrip
			//
			this.m_StatusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.m_ConnectionCountLabel});
			this.m_StatusStrip.Location = new System.Drawing.Point(0, 551);
			this.m_StatusStrip.Name = "m_StatusStrip";
			this.m_StatusStrip.Size = new System.Drawing.Size(792, 22);
			this.m_StatusStrip.TabIndex = 0;
			this.m_StatusStrip.Text = "m_StatusStrip";
			//
			// m_ConnectionCountLabel
			//
			this.m_ConnectionCountLabel.AutoSize = false;
			this.m_ConnectionCountLabel.AutoToolTip = true;
			this.m_ConnectionCountLabel.Name = "m_ConnectionCountLabel";
			this.m_ConnectionCountLabel.Size = new System.Drawing.Size(200, 17);
			this.m_ConnectionCountLabel.Text = "No Connections";
			this.m_ConnectionCountLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.m_ConnectionCountLabel.ToolTipText = "Total number of clients connected to your Moriarty server.";
			//
			// m_Split
			//
			this.m_Split.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_Split.Location = new System.Drawing.Point(0, 52);
			this.m_Split.Name = "m_Split";
			//
			// m_Split.Panel1
			//
			this.m_Split.Panel1.Controls.Add(this.m_ConnectionsPreferencesSplitPanel);
			//
			// m_Split.Panel2
			//
			this.m_Split.Panel2.Controls.Add(this.m_MoriartyConnectionControl);
			this.m_Split.Size = new System.Drawing.Size(792, 496);
			this.m_Split.SplitterDistance = 263;
			this.m_Split.TabIndex = 0;
			this.m_Split.TabStop = false;
			//
			// m_ConnectionsPreferencesSplitPanel
			//
			this.m_ConnectionsPreferencesSplitPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_ConnectionsPreferencesSplitPanel.Location = new System.Drawing.Point(3, 3);
			this.m_ConnectionsPreferencesSplitPanel.Name = "m_ConnectionsPreferencesSplitPanel";
			this.m_ConnectionsPreferencesSplitPanel.Orientation = System.Windows.Forms.Orientation.Horizontal;
			//
			// m_ConnectionsPreferencesSplitPanel.Panel1
			//
			this.m_ConnectionsPreferencesSplitPanel.Panel1.Controls.Add(this.m_ConnectionList);
			//
			// m_ConnectionsPreferencesSplitPanel.Panel2
			//
			this.m_ConnectionsPreferencesSplitPanel.Panel2.Controls.Add(this.m_UserSettings);
			this.m_ConnectionsPreferencesSplitPanel.Size = new System.Drawing.Size(257, 490);
			this.m_ConnectionsPreferencesSplitPanel.SplitterDistance = 220;
			this.m_ConnectionsPreferencesSplitPanel.TabIndex = 0;
			this.m_ConnectionsPreferencesSplitPanel.TabStop = false;
			//
			// m_ConnectionList
			//
			this.m_ConnectionList.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_ConnectionList.LargeImageList = this.m_ConnectionListIcons;
			this.m_ConnectionList.Location = new System.Drawing.Point(3, 3);
			this.m_ConnectionList.MultiSelect = false;
			this.m_ConnectionList.Name = "m_ConnectionList";
			this.m_ConnectionList.Size = new System.Drawing.Size(254, 214);
			this.m_ConnectionList.SmallImageList = this.m_ConnectionListIcons;
			this.m_ConnectionList.Sorting = System.Windows.Forms.SortOrder.Ascending;
			this.m_ConnectionList.TabIndex = 1;
			this.m_ConnectionList.UseCompatibleStateImageBehavior = false;
			this.m_ConnectionList.SelectedIndexChanged += new System.EventHandler(this.m_ConnectionList_SelectedIndexChanged);
			//
			// m_ConnectionListIcons
			//
			this.m_ConnectionListIcons.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("m_ConnectionListIcons.ImageStream")));
			this.m_ConnectionListIcons.TransparentColor = System.Drawing.Color.Transparent;
			this.m_ConnectionListIcons.Images.SetKeyName(0, "IconPC.png");
			this.m_ConnectionListIcons.Images.SetKeyName(1, "IconIOS.png");
			this.m_ConnectionListIcons.Images.SetKeyName(2, "IconAndroid.png");
			//
			// m_UserSettings
			//
			this.m_UserSettings.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_UserSettings.Location = new System.Drawing.Point(3, 3);
			this.m_UserSettings.Name = "m_UserSettings";
			this.m_UserSettings.Size = new System.Drawing.Size(254, 260);
			this.m_UserSettings.TabIndex = 2;
			//
			// m_MoriartyConnectionControl
			//
			this.m_MoriartyConnectionControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_MoriartyConnectionControl.Connection = null;
			this.m_MoriartyConnectionControl.Location = new System.Drawing.Point(3, 3);
			this.m_MoriartyConnectionControl.Manager = null;
			this.m_MoriartyConnectionControl.Name = "m_MoriartyConnectionControl";
			this.m_MoriartyConnectionControl.Size = new System.Drawing.Size(519, 490);
			this.m_MoriartyConnectionControl.TabIndex = 3;
			//
			// m_MainFormToolStrip
			//
			this.m_MainFormToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.m_TargetSelectionComboBox,
            this.m_RunTargetButton});
			this.m_MainFormToolStrip.Location = new System.Drawing.Point(0, 24);
			this.m_MainFormToolStrip.Name = "m_MainFormToolStrip";
			this.m_MainFormToolStrip.Size = new System.Drawing.Size(792, 25);
			this.m_MainFormToolStrip.TabIndex = 0;
			this.m_MainFormToolStrip.TabStop = true;
			this.m_MainFormToolStrip.Text = "m_MainFormToolStrip";
			//
			// m_TargetSelectionComboBox
			//
			this.m_TargetSelectionComboBox.Name = "m_TargetSelectionComboBox";
			this.m_TargetSelectionComboBox.Size = new System.Drawing.Size(121, 25);
			this.m_TargetSelectionComboBox.ToolTipText = "The target to launch when you click the Run Target button.";
			//
			// m_RunTargetButton
			//
			this.m_RunTargetButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			this.m_RunTargetButton.Enabled = false;
			this.m_RunTargetButton.Image = ((System.Drawing.Image)(resources.GetObject("m_RunTargetButton.Image")));
			this.m_RunTargetButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.m_RunTargetButton.Name = "m_RunTargetButton";
			this.m_RunTargetButton.Size = new System.Drawing.Size(23, 22);
			this.m_RunTargetButton.Text = "Run Target";
			this.m_RunTargetButton.ToolTipText = "Run the selected target.";
			this.m_RunTargetButton.Click += new System.EventHandler(this.m_RunTargetButton_Click);
			//
			// MoriartyMainForm
			//
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(792, 573);
			this.Controls.Add(this.m_MainFormToolStrip);
			this.Controls.Add(this.m_Split);
			this.Controls.Add(this.m_StatusStrip);
			this.Controls.Add(this.m_MenuStrip);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MainMenuStrip = this.m_MenuStrip;
			this.Name = "MoriartyMainForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Moriarty";
			this.m_MenuStrip.ResumeLayout(false);
			this.m_MenuStrip.PerformLayout();
			this.m_StatusStrip.ResumeLayout(false);
			this.m_StatusStrip.PerformLayout();
			this.m_Split.Panel1.ResumeLayout(false);
			this.m_Split.Panel2.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.m_Split)).EndInit();
			this.m_Split.ResumeLayout(false);
			this.m_ConnectionsPreferencesSplitPanel.Panel1.ResumeLayout(false);
			this.m_ConnectionsPreferencesSplitPanel.Panel2.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.m_ConnectionsPreferencesSplitPanel)).EndInit();
			this.m_ConnectionsPreferencesSplitPanel.ResumeLayout(false);
			this.m_MainFormToolStrip.ResumeLayout(false);
			this.m_MainFormToolStrip.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

        }
        #endregion

        private System.Windows.Forms.MenuStrip m_MenuStrip;
		private System.Windows.Forms.ToolStripMenuItem m_FileToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem m_ExitToolStripMenuItem;
		private System.Windows.Forms.StatusStrip m_StatusStrip;
		private System.Windows.Forms.ToolStripMenuItem m_HelpToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem m_AboutToolStripMenuItem;
		private System.Windows.Forms.SplitContainer m_Split;
		private System.Windows.Forms.ListView m_ConnectionList;
		private MoriartyConnectionControl m_MoriartyConnectionControl;
		private System.Windows.Forms.ImageList m_ConnectionListIcons;
		private System.Windows.Forms.PropertyGrid m_UserSettings;
		private System.Windows.Forms.ToolStripMenuItem m_SaveUserSettingsToolStripMenuItem;
		private System.Windows.Forms.ToolStripSeparator m_FileMenuToolStripSeperator;
		private System.Windows.Forms.ToolStripStatusLabel m_ConnectionCountLabel;
		private System.Windows.Forms.ToolStrip m_MainFormToolStrip;
		private System.Windows.Forms.ToolStripComboBox m_TargetSelectionComboBox;
		private System.Windows.Forms.ToolStripButton m_RunTargetButton;
		private System.Windows.Forms.SplitContainer m_ConnectionsPreferencesSplitPanel;
		private System.Windows.Forms.ToolStripMenuItem m_ViewToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem m_KeyboardCaptureWindowToolStripMenuItem;
    }
} // namespace Moriarty.UI
