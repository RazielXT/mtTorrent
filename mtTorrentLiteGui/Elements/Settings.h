#pragma once
#include "SettingsForm.h"

class AppCore;

class Settings
{
public:

	Settings(AppCore& core);

	void applySettings(GuiLite::SettingsForm^ form);
	void showSettingsForm();

private:

	AppCore& core;
};