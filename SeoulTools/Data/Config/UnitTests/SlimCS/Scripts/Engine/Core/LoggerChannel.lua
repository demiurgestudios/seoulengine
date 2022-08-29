-- Engine level names for built-in logging channels. Should be
-- kept in-sync with the enum LoggerChannel in Logger.h
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.

local LoggerChannel = class_static 'LoggerChannel'

LoggerChannel.Default = 'Default'

LoggerChannel.Analytics = 'Analytics'
LoggerChannel.Animation = 'Animation'
LoggerChannel.Assertion = 'Assertion'
LoggerChannel.Audio = 'Audio'
LoggerChannel.AudioEvents = 'AudioEvents'
LoggerChannel.Auth = 'Auth'
LoggerChannel.Automation = 'Automation'
LoggerChannel.Chat = 'Chat'
LoggerChannel.Commerce = 'Commerce'
LoggerChannel.Cooking = 'Cooking'
LoggerChannel.Core = 'Core'
LoggerChannel.Engine = 'Engine'
LoggerChannel.FailedGotoLabel = 'FailedGotoLabel'
LoggerChannel.FileIO = 'FileIO'
LoggerChannel.HTTP = 'HTTP'
LoggerChannel.Input = 'Input'
LoggerChannel.Loading = 'Loading'
LoggerChannel.Localization = 'Localization'
LoggerChannel.LocalizationWarning = 'LocalizationWarning'
LoggerChannel.Network = 'Network'
LoggerChannel.Notification = 'Notification'
LoggerChannel.Physics = 'Physics'
LoggerChannel.Render = 'Render'
LoggerChannel.Script = 'Script'
LoggerChannel.Server = 'Server'
LoggerChannel.State = 'State'
LoggerChannel.Tracking = 'Tracking'
LoggerChannel.TransformsErrors = 'TransformsErrors'
LoggerChannel.TransformsWarnings = 'TransformsWarnings'
LoggerChannel.TriggersAndEvents = 'TriggersAndEvents'
LoggerChannel.UI = 'UI'
LoggerChannel.UIDebug = 'UIDebug'
LoggerChannel.UnitTest = 'UnitTest'
LoggerChannel.Video = 'Video'
LoggerChannel.Warning = 'Warning'
return LoggerChannel
