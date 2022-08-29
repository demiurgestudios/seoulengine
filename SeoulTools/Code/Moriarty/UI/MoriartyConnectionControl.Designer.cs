// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

namespace Moriarty.UI
{
	partial class MoriartyConnectionControl
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
			if (null != m_Connection)
			{
				m_Connection.Dispose();
				m_Connection = null;
			}

			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Component Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.m_ClientLogTextBox = new System.Windows.Forms.RichTextBox();
			this.m_ServerLogTextBox = new System.Windows.Forms.RichTextBox();
			this.m_MoriartyConnectionTabControl = new System.Windows.Forms.TabControl();
			this.m_ManagerTabPage = new System.Windows.Forms.TabPage();
			this.m_ManagerLogTextBox = new System.Windows.Forms.RichTextBox();
			this.m_ServerTabPage = new System.Windows.Forms.TabPage();
			this.m_ClientTabPage = new System.Windows.Forms.TabPage();
			this.m_ConnectionName = new System.Windows.Forms.Label();
			this.m_MoriartyConnectionTabControl.SuspendLayout();
			this.m_ManagerTabPage.SuspendLayout();
			this.m_ServerTabPage.SuspendLayout();
			this.m_ClientTabPage.SuspendLayout();
			this.SuspendLayout();
			//
			// m_ClientLogTextBox
			//
			this.m_ClientLogTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_ClientLogTextBox.BackColor = System.Drawing.SystemColors.Control;
			this.m_ClientLogTextBox.Location = new System.Drawing.Point(0, 0);
			this.m_ClientLogTextBox.LinkClicked += new System.Windows.Forms.LinkClickedEventHandler(this.m_LogTextBox_LinkClicked);
			this.m_ClientLogTextBox.Name = "m_ClientLogTextBox";
			this.m_ClientLogTextBox.ReadOnly = true;
			this.m_ClientLogTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
			this.m_ClientLogTextBox.Size = new System.Drawing.Size(603, 416);
			this.m_ClientLogTextBox.TabIndex = 4;
			this.m_ClientLogTextBox.TabStop = false;
			//
			// m_ServerLogTextBox
			//
			this.m_ServerLogTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_ServerLogTextBox.BackColor = System.Drawing.SystemColors.Control;
			this.m_ServerLogTextBox.Location = new System.Drawing.Point(0, 0);
			this.m_ServerLogTextBox.LinkClicked += new System.Windows.Forms.LinkClickedEventHandler(this.m_LogTextBox_LinkClicked);
			this.m_ServerLogTextBox.Name = "m_ServerLogTextBox";
			this.m_ServerLogTextBox.ReadOnly = true;
			this.m_ServerLogTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
			this.m_ServerLogTextBox.Size = new System.Drawing.Size(603, 416);
			this.m_ServerLogTextBox.TabIndex = 5;
			this.m_ServerLogTextBox.TabStop = false;
			//
			// m_MoriartyConnectionTabControl
			//
			this.m_MoriartyConnectionTabControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_MoriartyConnectionTabControl.Controls.Add(this.m_ManagerTabPage);
			this.m_MoriartyConnectionTabControl.Controls.Add(this.m_ServerTabPage);
			this.m_MoriartyConnectionTabControl.Controls.Add(this.m_ClientTabPage);
			this.m_MoriartyConnectionTabControl.Location = new System.Drawing.Point(3, 29);
			this.m_MoriartyConnectionTabControl.Name = "m_MoriartyConnectionTabControl";
			this.m_MoriartyConnectionTabControl.SelectedIndex = 0;
			this.m_MoriartyConnectionTabControl.Size = new System.Drawing.Size(611, 442);
			this.m_MoriartyConnectionTabControl.TabIndex = 1;
			//
			// m_ManagerTabPage
			//
			this.m_ManagerTabPage.Controls.Add(this.m_ManagerLogTextBox);
			this.m_ManagerTabPage.Location = new System.Drawing.Point(4, 22);
			this.m_ManagerTabPage.Name = "m_ManagerTabPage";
			this.m_ManagerTabPage.Size = new System.Drawing.Size(603, 416);
			this.m_ManagerTabPage.TabIndex = 2;
			this.m_ManagerTabPage.Text = "Manager";
			this.m_ManagerTabPage.ToolTipText = "Log window for the Moriarty Manager, which handles connection management and glob" +
    "al tasks such as cooking.";
			this.m_ManagerTabPage.UseVisualStyleBackColor = true;
			//
			// m_ManagerLogTextBox
			//
			this.m_ManagerLogTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
            | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
			this.m_ManagerLogTextBox.BackColor = System.Drawing.SystemColors.Control;
			this.m_ManagerLogTextBox.Location = new System.Drawing.Point(0, 0);
			this.m_ManagerLogTextBox.LinkClicked += new System.Windows.Forms.LinkClickedEventHandler(this.m_LogTextBox_LinkClicked);
			this.m_ManagerLogTextBox.Name = "m_ManagerLogTextBox";
			this.m_ManagerLogTextBox.ReadOnly = true;
			this.m_ManagerLogTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
			this.m_ManagerLogTextBox.Size = new System.Drawing.Size(603, 416);
			this.m_ManagerLogTextBox.TabIndex = 7;
			this.m_ManagerLogTextBox.TabStop = false;
			//
			// m_ServerTabPage
			//
			this.m_ServerTabPage.Controls.Add(this.m_ServerLogTextBox);
			this.m_ServerTabPage.Location = new System.Drawing.Point(4, 22);
			this.m_ServerTabPage.Name = "m_ServerTabPage";
			this.m_ServerTabPage.Padding = new System.Windows.Forms.Padding(3);
			this.m_ServerTabPage.Size = new System.Drawing.Size(603, 416);
			this.m_ServerTabPage.TabIndex = 0;
			this.m_ServerTabPage.Text = "Server";
			this.m_ServerTabPage.ToolTipText = "Log window for messages generated by the Moriarty UI for the currently selected c" +
    "onnection.";
			this.m_ServerTabPage.UseVisualStyleBackColor = true;
			//
			// m_ClientTabPage
			//
			this.m_ClientTabPage.Controls.Add(this.m_ClientLogTextBox);
			this.m_ClientTabPage.Location = new System.Drawing.Point(4, 22);
			this.m_ClientTabPage.Name = "m_ClientTabPage";
			this.m_ClientTabPage.Padding = new System.Windows.Forms.Padding(3);
			this.m_ClientTabPage.Size = new System.Drawing.Size(603, 416);
			this.m_ClientTabPage.TabIndex = 1;
			this.m_ClientTabPage.Text = "Client";
			this.m_ClientTabPage.ToolTipText = "Log window for messages generated from the client application for the currently s" +
    "elected connection.";
			this.m_ClientTabPage.UseVisualStyleBackColor = true;
			//
			// m_ConnectionName
			//
			this.m_ConnectionName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.m_ConnectionName.Location = new System.Drawing.Point(8, 6);
			this.m_ConnectionName.Name = "m_ConnectionName";
			this.m_ConnectionName.Size = new System.Drawing.Size(168, 14);
			this.m_ConnectionName.TabIndex = 2;
			this.m_ConnectionName.Text = "No Selection";
			//
			// MoriartyConnectionControl
			//
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.m_ConnectionName);
			this.Controls.Add(this.m_MoriartyConnectionTabControl);
			this.Name = "MoriartyConnectionControl";
			this.Size = new System.Drawing.Size(617, 474);
			this.m_MoriartyConnectionTabControl.ResumeLayout(false);
			this.m_ManagerTabPage.ResumeLayout(false);
			this.m_ManagerTabPage.PerformLayout();
			this.m_ServerTabPage.ResumeLayout(false);
			this.m_ServerTabPage.PerformLayout();
			this.m_ClientTabPage.ResumeLayout(false);
			this.m_ClientTabPage.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.RichTextBox m_ClientLogTextBox;
		private System.Windows.Forms.RichTextBox m_ServerLogTextBox;
		private System.Windows.Forms.TabControl m_MoriartyConnectionTabControl;
		private System.Windows.Forms.TabPage m_ServerTabPage;
		private System.Windows.Forms.TabPage m_ClientTabPage;
		private System.Windows.Forms.TabPage m_ManagerTabPage;
		private System.Windows.Forms.RichTextBox m_ManagerLogTextBox;
		private System.Windows.Forms.Label m_ConnectionName;
	}
} // namespace Moriarty.UI
