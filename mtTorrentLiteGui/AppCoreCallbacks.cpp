#include "AppCoreCallbacks.h"
#include "AppCore.h"
#include <fstream>
#include "MainForm.h"
#include "../mtTorrent/utils/HexEncoding.h"

extern AppCore core;

void onButtonClick(ButtonId id)
{
	onButtonClick(id, nullptr);
}
void onButtonClick(ButtonId id, System::String^ param)
{
	core.onButtonClick(id, param);
}
TorrentCtxMenuInfo getTorrentContexMenuInfo()
{
	return core.getTorrentContexMenuInfo();
}
void applySettings(System::Windows::Forms::Form^ form)
{
	core.settings.applySettings((GuiLite::SettingsForm^)form);
}
System::String^ getUpnpInfo()
{
	return core.getUpnpInfo();
}
void scheduleTorrent(System::DateTime time)
{
	core.scheduler.scheduleTorrent(time, core.firstSelectedHash);
}
void fillFilesSelectionForm()
{
	core.fileSelection.fillFilesSelectionForm();
}
void fileSelectionChanged(int id, bool selected)
{
	core.fileSelection.fileSelectionChanged(id, selected);
}
void filePriorityChanged(const std::vector<int>& ids, System::String^ priority)
{
	core.fileSelection.filePriorityChanged(ids, priority);
}
void fileProgressSelectionChanged(int row)
{
	core.torrentsView.piecesProgress.fileSelectionChanged(row);
}
void forceGuiRefresh()
{
	core.forceRefresh = true;
}
System::String^ fileSelectionValidatePath(System::String^ path)
{
	return core.fileSelection.validatePath(path);
}
void torrentsGridSorted()
{
	core.torrentsView.updateList();
}

using namespace System::Xml;

UserWindowState^ getWindowState()
{
	UserWindowState^ state = GuiLite::MainForm::instance->windowState;

	if (!state)
	{
		state = gcnew UserWindowState();
		System::Xml::XmlDocument^ doc = gcnew System::Xml::XmlDocument;

		if (System::IO::File::Exists("./data/window.xml"))
		{
			try
			{
				doc->Load("./data/window.xml");

				if (auto user = doc->GetElementsByTagName("window"))
				{
					for each(XmlNode^ e in user[0]->ChildNodes)
					{
						if (e->Name == "maximized")
							System::Boolean::TryParse(e->InnerText, state->maximized);
						else if (e->Name == "height")
							System::Int32::TryParse(e->InnerText, state->height);
						else if (e->Name == "width")
							System::Int32::TryParse(e->InnerText, state->width);
						else if (e->Name == "splitterDistance")
							System::Int32::TryParse(e->InnerText, state->splitterDistance);
						else if (e->Name == "addPeer")
							state->addPeer = e->InnerText;
						else if (e->Name == "torrentsGrid")
						{
							for each (XmlNode^ el in e->ChildNodes)
							{
								if (el->Name == "sortColumn")
									state->torrentSortColumn = el->InnerText;
								else if (el->Name == "sortDesc")
									System::Boolean::TryParse(el->InnerText, state->torrentSortColumnDesc);
								else if (el->Name == "columns")
								{
									state->torrentColumns = gcnew System::Collections::Generic::Dictionary<System::String^, int>();

									for each (XmlNode ^ column in el->ChildNodes)
									{
										if (column->Name == "column")
										{
											System::String^ name;
											System::String^ width;
											for each (XmlNode ^ e in column->ChildNodes)
											{
												if (e->Name == "name")
													name = e->InnerText;
												else if (e->Name == "width")
													width = e->InnerText;
											}

											if (name && width)
												state->torrentColumns[name] = System::Convert::ToInt32(width);
										}
									}
								}
							}
						}
						else if (e->Name == "selectedTorrent")
						{
							core.torrentsView.loadState(e);
						}
					}
				}
			}
			catch (System::Exception^ ex)
			{
				::MessageBoxA(NULL, getUtf8String(ex->Message).data(), "Window state load failure", MB_OK);
			}
		}

		GuiLite::MainForm::instance->windowState = state;
	}

	return state;
}

void saveWindowState()
{
	UserWindowState^ state = GuiLite::MainForm::instance->windowState;

	if (state)
	{
		XmlDocument^ doc = gcnew XmlDocument();

		XmlElement^ root = doc->CreateElement("window");
		doc->AppendChild(root);

		XmlElement^ e;

		if (state->addPeer)
		{
			e = doc->CreateElement("addPeer");
			e->InnerText = state->addPeer;
			root->AppendChild(e);
		}

		{
			e = doc->CreateElement("maximized");
			e->InnerText = state->maximized.ToString();
			root->AppendChild(e);

			e = doc->CreateElement("height");
			e->InnerText = state->height.ToString();
			root->AppendChild(e);

			e = doc->CreateElement("width");
			e->InnerText = state->width.ToString();
			root->AppendChild(e);

			e = doc->CreateElement("splitterDistance");
			e->InnerText = state->splitterDistance.ToString();
			root->AppendChild(e);

			{
				XmlElement^ torrents = doc->CreateElement("torrentsGrid");

				if (state->torrentSortColumn)
				{
					e = doc->CreateElement("sortColumn");
					e->InnerText = state->torrentSortColumn;
					torrents->AppendChild(e);

					e = doc->CreateElement("sortDesc");
					e->InnerText = state->torrentSortColumnDesc.ToString();
					torrents->AppendChild(e);
				}

				if (state->torrentColumns)
				{
					XmlElement^ columns = doc->CreateElement("columns");

					for each (System::Collections::Generic::KeyValuePair<System::String^, int>^ col in state->torrentColumns)
					{
						XmlElement^ ce = doc->CreateElement("column");
						
						XmlElement^ e = doc->CreateElement("name");
						e->InnerText = col->Key;
						ce->AppendChild(e);

						e = doc->CreateElement("width");
						e->InnerText = col->Value.ToString();
						ce->AppendChild(e);

						columns->AppendChild(ce);
					}

					torrents->AppendChild(columns);
				}

				root->AppendChild(torrents);
			}

			if (core.selected)
			{
				XmlElement^ selection = doc->CreateElement("selectedTorrent");

				auto t = hexToString(core.firstSelectedHash, 20);
				e = doc->CreateElement("id");
				e->InnerText = gcnew System::String(t.c_str());
				selection->AppendChild(e);

				core.torrentsView.saveState(selection);

				root->AppendChild(selection);
			}
		}

		try
		{
			doc->Save("./data/window.xml");
		}
		catch (System::Exception^ ex)
		{
			::MessageBoxA(NULL, getUtf8String(ex->Message).data(), "Window state save failure", MB_OK);
		}

	}
}

float bytesToNumber(System::String^ str)
{
	float value = 0;
	int i = str->IndexOf(' ');
	if (i != -1)
		float::TryParse(str->Substring(0, i), value);

	if (str->Contains("GB"))
		value *= 1024 * 1024 * 1024;
	else if (str->Contains("MB"))
		value *= 1024 * 1024;
	else if (str->Contains("KB"))
		value *= 1024;

	return value;
}
