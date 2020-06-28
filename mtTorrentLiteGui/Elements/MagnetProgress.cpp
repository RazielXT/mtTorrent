#include "MagnetProgress.h"
#include "MagnetInputForm.h"
#include "../AppCore.h"
#include "../../mtTorrent/utils/HexEncoding.h"
#include "../../mtTorrent/Public/BinaryInterface.h"

MagnetProgress::MagnetProgress(AppCore& c) : core(c)
{
}

mtt::Status MagnetProgress::addTorrentFromMetadata(const char* magnetPtr)
{
	auto status = core.IoctlFunc(mtBI::MessageId::AddFromMetadata, magnetPtr, state.hash);
	core.handleTorrentAddResponse(status, state.hash);

	return status;
}

void MagnetProgress::update()
{
	if (GuiLite::MagnetInputForm::instance)
	{
		if (state.magnetLinkSequence >= Sequence::Added)
		{
			if (state.magnetLinkSequence == Sequence::AddedWithLogs)
			{
				mtBI::MagnetLinkProgressLogsRequest logsRequest;
				logsRequest.start = state.lastMagnetLinkLogCount;
				memcpy(logsRequest.hash, state.hash, 20);
				mtBI::MagnetLinkProgressLogsResponse logsResponse;

				if (core.IoctlFunc(mtBI::MessageId::GetMagnetLinkProgressLogs, &logsRequest, &logsResponse) == mtt::Status::Success)
				{
					for (auto& l : logsResponse.logs)
					{
						GuiLite::MagnetInputForm::instance->logsTextBox->Text += gcnew System::String(l.data);
						GuiLite::MagnetInputForm::instance->logsTextBox->Text += "\n";
					}

					state.lastMagnetLinkLogCount = (uint32_t)logsResponse.fullcount;
				}
			}

			mtBI::MagnetLinkProgress progress;
			auto status = core.IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, state.hash, &progress);
			if (status == mtt::Status::Success)
			{
				if (progress.progress > 1.0f)
					progress.progress = 1.0f;

				GuiLite::MagnetInputForm::instance->progressBarMagnet->Value = (int)(progress.progress * 100);

				if (progress.finished)
				{
					if (state.magnetLinkSequence == Sequence::Added)
						GuiLite::MagnetInputForm::instance->Close();
					else
					{
						GuiLite::MagnetInputForm::instance->labelText->Text = "Finished";
						GuiLite::MagnetInputForm::instance->progressBarMagnet->Visible = false;
						state.magnetLinkSequence = Sequence::Finished;
					}
				}
			}
			else
			{
				GuiLite::MagnetInputForm::instance->progressBarMagnet->Visible = false;
				GuiLite::MagnetInputForm::instance->labelText->Text = status == mtt::Status::E_NoData ? "No magnet data" : "Error";
			}
		}
	}
}

void MagnetProgress::onButtonClick(ButtonId id)
{
	if (id == ButtonId::AddTorrentMagnet)
	{
		state.magnetLinkSequence = Sequence::Input;
		GuiLite::MagnetInputForm form;
		form.progressBarMagnet->Visible = false;
		form.textBoxMagnet->Text = GetClipboardText();
		form.ShowDialog();
		GuiLite::MagnetInputForm::instance = nullptr;
		state.magnetLinkSequence = Sequence::Closed;
		state.lastMagnetLinkLogCount = 0;
	}
	else if (id == ButtonId::MagnetLogs)
	{
		if (!core.selected)
			return;

		mtBI::MagnetLinkProgress magnetProgress;
		memcpy(state.hash, core.firstSelectedHash, 20);
		if (!GuiLite::MagnetInputForm::instance && core.IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, state.hash, &magnetProgress) == mtt::Status::Success)
		{
			GuiLite::MagnetInputForm form;
			form.magnetFormButton->Text = "Logs";
			form.magnetFormButton->Enabled = false;
			form.logsTextBox->Visible = true;
			form.textBoxMagnet->Text = gcnew System::String(hexToString(state.hash, 20).data());
			form.textBoxMagnet->ReadOnly = true;
			state.magnetLinkSequence = Sequence::AddedWithLogs;
			form.ShowDialog();
			GuiLite::MagnetInputForm::instance = nullptr;
			state.magnetLinkSequence = Sequence::Closed;
			state.lastMagnetLinkLogCount = 0;
		}
	}

	if (GuiLite::MagnetInputForm::instance)
	{
		if (id == ButtonId::MagnetButton)
		{
			if (GuiLite::MagnetInputForm::instance->magnetFormButton->Text->StartsWith("Add"))
			{
				if (GuiLite::MagnetInputForm::instance->textBoxMagnet->Text->Length == 0)
					return;

				if (state.magnetLinkSequence > Sequence::Input)
					return;

				auto magnetStr = getUtf8String(GuiLite::MagnetInputForm::instance->textBoxMagnet->Text);
				auto status = addTorrentFromMetadata(magnetStr.data());

				if (status == mtt::Status::Success)
				{
					GuiLite::MagnetInputForm::instance->textBoxMagnet->ReadOnly = true;
					GuiLite::MagnetInputForm::instance->progressBarMagnet->Visible = true;
					GuiLite::MagnetInputForm::instance->labelText->Text = "";
					state.magnetLinkSequence = Sequence::Added;

					GuiLite::MagnetInputForm::instance->magnetFormButton->Text = "Logs";
				}
				else if (status == mtt::Status::I_AlreadyExists || status == mtt::Status::I_Merged)
				{
					GuiLite::MagnetInputForm::instance->Close();
				}
				else
				{
					GuiLite::MagnetInputForm::instance->progressBarMagnet->Visible = false;
					GuiLite::MagnetInputForm::instance->labelText->Text = "Invalid magnet link";
				}
			}
			else if (GuiLite::MagnetInputForm::instance->magnetFormButton->Text->StartsWith("Logs"))
			{
				GuiLite::MagnetInputForm::instance->magnetFormButton->Enabled = false;
				GuiLite::MagnetInputForm::instance->logsTextBox->Visible = true;
				state.magnetLinkSequence = Sequence::AddedWithLogs;
			}
		}
	}
}

