#include "FileSelection.h"
#include "FileSelectionForm.h"
#include "../Utils/Utils.h"
#include "../AppCore.h"
#include "MainForm.h"

FileSelection::FileSelection(AppCore& c) : core(c)
{
}

void FileSelection::updateSelectionFormFooter()
{
	uint64_t selectedSize = 0;
	uint64_t fullsize = 0;
	uint32_t selectedCount = 0;

	for (auto& f : state.info.files)
	{
		if (f.selected)
		{
			selectedCount++;
			selectedSize += f.size;
		}

		fullsize += f.size;
	}

	System::String^ txt = gcnew System::String("Selected ");
	txt += int(selectedCount).ToString();
	txt += "/";
	txt += int(state.info.files.size()).ToString();
	txt += " (";
	txt += formatBytes(selectedSize);
	txt += "/";
	txt += formatBytes(fullsize);
	txt += ")";

	GuiLite::FileSelectionForm::instance->infoLabel->Text = txt;
	GuiLite::FileSelectionForm::instance->validate();
}

void FileSelection::fileSelectionChanged(int id, bool selected)
{
	if (id >= (int)state.info.files.size())
		return;

	state.info.files[id].selected = selected;

	updateSelectionFormFooter();
}

void FileSelection::filePriorityChanged(const std::vector<int>& filesIdx, System::String^ value)
{
	uint8_t priority = 50;
	if (value == "Low")
		priority = 10;
	else if (value == "High")
		priority = 90;

	for (auto i : filesIdx)
	{
		state.info.files[i].priority = priority;
	}
	state.priorityChanged = true;
}

void FileSelection::setSelectionForAllFiles(bool selected)
{
	GuiLite::FileSelectionForm::instance->initialized = false;
	auto rows = GuiLite::FileSelectionForm::instance->filesGridView->Rows;
	int i = 0;

	for (auto& s : state.info.files)
	{
		s.selected = selected;
		rows[i++]->Cells[1]->Value = selected;
	}
	GuiLite::FileSelectionForm::instance->initialized = true;
	updateSelectionFormFooter();
}

static System::String^ formatPriorityToString(uint8_t p)
{
	if (p == 50)
		return "Normal";
	else if (p > 50)
		return "High";
	else //if (p < 50)
		return "Low";
}

void FileSelection::fillFilesSelectionForm()
{
	auto form = GuiLite::FileSelectionForm::instance;
	auto& info = state.info;

	mtBI::TorrentInfo tmp;
	if (core.IoctlFunc(mtBI::MessageId::GetTorrentInfo, state.hash, &tmp) != mtt::Status::Success || tmp.files.empty())
		return;

	info = tmp;

	auto list = form->filesGridView;
	list->Rows->Add((int)info.files.size());

	int i = 0;
	for (auto& f : info.files)
	{
		auto row = gcnew cli::array<System::Object^>(5) {
			int(i).ToString(),
				f.selected,
				gcnew System::String(f.name.data, 0, (int)f.name.length, System::Text::Encoding::UTF8),
				formatBytes(f.size),
				formatPriorityToString(f.priority)
		};

		list->Rows[i]->SetValues(row);
		i++;
	}

	list->Sort(list->Columns[2], System::ComponentModel::ListSortDirection::Ascending);

	form->Text = gcnew System::String(info.name.data, 0, (int)info.name.length, System::Text::Encoding::UTF8);
	form->textBoxPath->Text = gcnew System::String(info.downloadLocation.data, 0, (int)info.downloadLocation.length, System::Text::Encoding::UTF8);

	form->labelError->Visible = false;
	form->checkBoxStart->Visible = state.added;
	state.priorityChanged = false;

	mtBI::FilesAllocation tmp2;
	if (core.IoctlFunc(mtBI::MessageId::GetFilesAllocation, state.hash, &tmp2) != mtt::Status::Success || tmp2.files.size() != state.info.files.size())
		return;

	currentAllocation.assign(tmp2.files.data(), tmp2.files.data() + tmp2.files.size());

	updateSelectionFormFooter();
}

void FileSelection::showFilesSelectionForm(const uint8_t* hash, bool added)
{
	if (GuiLite::FileSelectionForm::instance)
		return;

	memcpy(state.hash, hash, 20);
	state.added = added;

	GuiLite::MainForm::instance->showFilesSelectionForm();
}

void FileSelection::onButtonClick(ButtonId id)
{
	if (id == ButtonId::SelectFiles)
	{
		showFilesSelectionForm(core.firstSelectedHash, false);
		return;
	}

	auto form = GuiLite::FileSelectionForm::instance;
	if (!form)
		return;

	if (id == ButtonId::SelectionOk)
	{
		if (form->textBoxPath->Text->Length > 0)
		{
			//set torrent files location, moving files if path changed
			mtBI::TorrentSetPathRequest path;
			memcpy(path.hash, state.hash, 20);
			path.moveFiles = true;
			path.path = getUtf8String(form->textBoxPath->Text);
			auto status = core.IoctlFunc(mtBI::MessageId::SetTorrentPath, &path, nullptr);

			if (status == mtt::Status::E_NotEmpty)
			{
				auto result = ::MessageBox((HWND)form->Handle.ToInt64(), L"Folder not empty, inherit containing files?", L"Torrent folder", MB_YESNO);

				if (result == IDYES)
				{
					path.moveFiles = false;
					status = core.IoctlFunc(mtBI::MessageId::SetTorrentPath, &path, nullptr);
				}
			}

			if (status != mtt::Status::Success)
			{
				if (status == mtt::Status::E_InvalidPath)
					form->labelError->Text = "Invalid path";
				if (status == mtt::Status::E_NotEmpty)
					form->labelError->Text = "Path not empty";
				else
					form->labelError->Text = "Error setting location path";

				form->labelError->Visible = true;
				return;
			}

			core.selectionChanged = true;
		}

		if (!state.info.files.empty())
		{
			mtBI::TorrentFilesSelectionRequest selection;
			memcpy(selection.hash, state.hash, 20);
			selection.selection.reserve(state.info.files.size());

			for (auto& s : state.info.files)
			{
				mtBI::FileSelectionRequest f;
				f.selected = s.selected;
				selection.selection.add(f);
			}

			core.IoctlFunc(mtBI::MessageId::SetTorrentFilesSelection, &selection, nullptr);
		}

		if (state.priorityChanged)
		{
			mtBI::TorrentFilesPriorityRequest request;
			memcpy(request.hash, state.hash, 20);
			request.priority.reserve(state.info.files.size());

			for (auto& f : state.info.files)
			{
				request.priority.add(f.priority);
			}

			core.IoctlFunc(mtBI::MessageId::SetTorrentFilesPriority, &request, nullptr);
		}

		if (form->checkBoxStart->Visible && form->checkBoxStart->Checked)
			core.IoctlFunc(mtBI::MessageId::Start, state.hash, nullptr);
		else
			core.torrentsView.updateList();

		form->Close();
	}
	else if (id == ButtonId::SelectionCancel)
	{
		if (state.added)
			core.removeTorrent(state.hash, false);

		form->Close();
	}
	else if (id == ButtonId::SelectionAll)
	{
		setSelectionForAllFiles(true);
	}
	else if (id == ButtonId::SelectionNone)
	{
		setSelectionForAllFiles(false);
	}
}

System::String^ FileSelection::validatePath(System::String^ path)
{
	if (!path->Contains(":"))
		return "Invalid path";

	System::IO::DriveInfo^ drive = nullptr;
	System::IO::FileInfo^ file = gcnew System::IO::FileInfo(path);
	if (!file->Directory)
		drive = gcnew System::IO::DriveInfo(path);
	else
		drive = gcnew System::IO::DriveInfo(file->Directory->Root->FullName);

	if (!drive->IsReady)
		return "Invalid drive";

	uint64_t neededFreeSize = 0;
	for (size_t i = 0; i < currentAllocation.size(); i++)
	{
		auto& file = state.info.files[i];
		if (file.selected && file.size > currentAllocation[i])
		{
			neededFreeSize += file.size - currentAllocation[i];
		}
	}

	if ((uint64_t)drive->AvailableFreeSpace < neededFreeSize)
	{
		auto txt = gcnew System::String("Not enough space, Available: ");
		txt += formatBytes((uint64_t)drive->AvailableFreeSpace);
		txt += ", Needed: ";
		txt += formatBytes((uint64_t)neededFreeSize);

		return txt;
	}

	return "";
}
