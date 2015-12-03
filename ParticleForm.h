#pragma once

extern void ResetParticles();
extern int NumParticles;
namespace ParticleTool {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for ParticleForm
	/// </summary>
	public ref class ParticleForm : public System::Windows::Forms::Form
	{
	public:
		ParticleForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~ParticleForm()
		{
			if (components)
			{
				delete components;
			}
		}
	public: System::Windows::Forms::Panel^  renderPanel;
	private: System::Windows::Forms::Button^  exitButton;
	private: System::Windows::Forms::Button^  resetButton;
	private: System::Windows::Forms::TrackBar^  particleTrackBar;



	private:
		
	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->renderPanel = (gcnew System::Windows::Forms::Panel());
			this->exitButton = (gcnew System::Windows::Forms::Button());
			this->resetButton = (gcnew System::Windows::Forms::Button());
			this->particleTrackBar = (gcnew System::Windows::Forms::TrackBar());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->particleTrackBar))->BeginInit();
			this->SuspendLayout();
			// 
			// renderPanel
			// 
			this->renderPanel->BorderStyle = System::Windows::Forms::BorderStyle::Fixed3D;
			this->renderPanel->Location = System::Drawing::Point(12, 12);
			this->renderPanel->Name = L"renderPanel";
			this->renderPanel->Size = System::Drawing::Size(1084, 814);
			this->renderPanel->TabIndex = 0;
			// 
			// exitButton
			// 
			this->exitButton->Location = System::Drawing::Point(1225, 13);
			this->exitButton->Name = L"exitButton";
			this->exitButton->Size = System::Drawing::Size(75, 23);
			this->exitButton->TabIndex = 1;
			this->exitButton->Text = L"Exit";
			this->exitButton->UseVisualStyleBackColor = true;
			this->exitButton->Click += gcnew System::EventHandler(this, &ParticleForm::exitButton_Click);
			// 
			// resetButton
			// 
			this->resetButton->Location = System::Drawing::Point(1120, 13);
			this->resetButton->Name = L"resetButton";
			this->resetButton->Size = System::Drawing::Size(75, 23);
			this->resetButton->TabIndex = 2;
			this->resetButton->Text = L"reset";
			this->resetButton->UseVisualStyleBackColor = true;
			this->resetButton->Click += gcnew System::EventHandler(this, &ParticleForm::resetButton_Click);
			// 
			// particleTrackBar
			// 
			this->particleTrackBar->LargeChange = 1000;
			this->particleTrackBar->Location = System::Drawing::Point(1120, 63);
			this->particleTrackBar->Maximum = 200000;
			this->particleTrackBar->Minimum = 100;
			this->particleTrackBar->Name = L"particleTrackBar";
			this->particleTrackBar->Size = System::Drawing::Size(180, 45);
			this->particleTrackBar->SmallChange = 100;
			this->particleTrackBar->TabIndex = 3;
			this->particleTrackBar->TickFrequency = 1000;
			this->particleTrackBar->TickStyle = System::Windows::Forms::TickStyle::TopLeft;
			this->particleTrackBar->Value = 100000;
			this->particleTrackBar->ValueChanged += gcnew System::EventHandler(this, &ParticleForm::particleTrackBar_ValueChanged);
			// 
			// ParticleForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1312, 838);
			this->Controls->Add(this->particleTrackBar);
			this->Controls->Add(this->resetButton);
			this->Controls->Add(this->exitButton);
			this->Controls->Add(this->renderPanel);
			this->Name = L"ParticleForm";
			this->Text = L"ParticleForm";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->particleTrackBar))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

		private: System::Void exitButton_Click(System::Object^  sender, System::EventArgs^  e)
		{
			Close();
		}
	private: System::Void resetButton_Click(System::Object^  sender, System::EventArgs^  e) {
		ResetParticles();
	}
	private: System::Void particleTrackBar_ValueChanged(System::Object^  sender, System::EventArgs^  e) {
		NumParticles = particleTrackBar->Value;
		ResetParticles();
	}
};
}
