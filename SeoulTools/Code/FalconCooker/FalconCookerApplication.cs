// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace FalconCooker
{
	public static class Application
	{
		public static int Main(string[] asArgs)
		{
			try
			{
				string sInputFilename = string.Empty;
				string sOutputFilename = string.Empty;
				string sImagesFolder = string.Empty;
				string sInOnlyImgDir = string.Empty;
				string sImagePrefix = string.Empty;
				string sInOnlyImagePrefix = string.Empty;
				bool bDoNotAllowLossyCompression = false;
				int iMinSwfVersion = -1;

				for (int i = 0; i < asArgs.Length; ++i)
				{
					switch (asArgs[i])
					{
						case "-o":
							if (!string.IsNullOrEmpty(sOutputFilename))
							{
								Console.Error.WriteLine("Output filename cannot be specified twice.");
								goto error;
							}

							if (i + 1 >= asArgs.Length)
							{
								Console.Error.WriteLine("-o must be followed by an output filename.");
								goto error;
							}

							++i;

							sOutputFilename = asArgs[i];
							break;

						case "-in_only_img_dir":
							if (!string.IsNullOrEmpty(sInOnlyImgDir))
							{
								Console.Error.WriteLine("Input only image directory cannot be specified twice.");
								goto error;
							}

							if (i + 1 >= asArgs.Length)
							{
								Console.Error.WriteLine("-in_only_img_dir must be followed by a directory path.");
								goto error;
							}
							++i;

							sInOnlyImgDir = asArgs[i];
							break;

						case "-img_dir":
							if (!string.IsNullOrEmpty(sImagesFolder))
							{
								Console.Error.WriteLine("Images directory cannot be specified twice.");
								goto error;
							}

							if (i + 1 >= asArgs.Length)
							{
								Console.Error.WriteLine("-img_dir must be followed by a directory path.");
								goto error;
							}
							++i;

							sImagesFolder = asArgs[i];
							break;

						case "-image_prefix":
							if (!string.IsNullOrEmpty(sImagePrefix))
							{
								Console.Error.WriteLine("Image prefix cannot be specified twice.");
								goto error;
							}

							if (i + 1 >= asArgs.Length)
							{
								Console.Error.WriteLine("-image_prefix must be followed by a text prefix.");
								goto error;
							}
							++i;

							sImagePrefix = asArgs[i];
							break;

						case "-in_only_image_prefix":
							if (!string.IsNullOrEmpty(sInOnlyImagePrefix))
							{
								Console.Error.WriteLine("Input only image prefix cannot be specified twice.");
								goto error;
							}

							if (i + 1 >= asArgs.Length)
							{
								Console.Error.WriteLine("-in_only_image_prefix must be followed by a text prefix.");
								goto error;
							}
							++i;

							sInOnlyImagePrefix = asArgs[i];
							break;

						case "-min_swf_version":
							if (iMinSwfVersion >= 0)
							{
								Console.Error.WriteLine("Minimum SWF version cannot be specified twice.");
								goto error;
							}

							if (i + 1 >= asArgs.Length)
							{
								Console.Error.WriteLine("-min_swf_version must be followed by a minimum version number.");
								goto error;
							}
							++i;

							if (!int.TryParse(asArgs[i], out iMinSwfVersion))
							{
								Console.Error.WriteLine("-min_swf_version must be followed by a minimum version number integer.");
								goto error;
							}
							break;

						case "-no_lossy":
							bDoNotAllowLossyCompression = true;
							break;

						default:
							if (!string.IsNullOrEmpty(sInputFilename))
							{
								Console.Error.WriteLine("Input filename cannot be specified twice.");
								goto error;
							}

							sInputFilename = asArgs[i];
							break;
					}
				}

				if (string.IsNullOrEmpty(sImagesFolder) ||
					string.IsNullOrEmpty(sInputFilename) ||
					string.IsNullOrEmpty(sOutputFilename))
				{
					Console.Error.WriteLine("Must specify an input filename, output filename, and images directory.");
					goto error;
				}

				if (!Path.IsPathRooted(sInputFilename))
				{
					sInputFilename = Path.Combine(Environment.CurrentDirectory, sInputFilename);
				}

				if (!Path.IsPathRooted(sOutputFilename))
				{
					sOutputFilename = Path.Combine(Environment.CurrentDirectory, sOutputFilename);
				}

				if (!Path.IsPathRooted(sImagesFolder))
				{
					sImagesFolder = Path.Combine(Environment.CurrentDirectory, sImagesFolder);
				}

				if (!string.IsNullOrEmpty(sInOnlyImgDir) && !Path.IsPathRooted(sInOnlyImgDir))
				{
					sInOnlyImgDir = Path.Combine(Environment.CurrentDirectory, sInOnlyImgDir);
				}

				if (!sImagesFolder.EndsWith(Path.DirectorySeparatorChar.ToString()))
				{
					sImagesFolder = sImagesFolder + Path.DirectorySeparatorChar;
				}

				if (!string.IsNullOrEmpty(sInOnlyImgDir) && !sInOnlyImgDir.EndsWith(Path.DirectorySeparatorChar.ToString()))
				{
					sInOnlyImgDir = sInOnlyImgDir + Path.DirectorySeparatorChar;
				}

				// Create directory dependencies.
				Directory.CreateDirectory(sImagesFolder);
				Directory.CreateDirectory(Path.GetDirectoryName(sOutputFilename));

				// TODO: Use an endian swapping BinaryWriter when appropriate.
				bool bSuccess = false;
				using (BinaryWriter writer = new BinaryWriter(File.Open(sOutputFilename, FileMode.Create, FileAccess.Write)))
				{
					// Create a cooker - if construction is successful, writing was successful.
					using (SWFCooker swfCooker = new SWFCooker(sInputFilename, writer, sImagesFolder, sInOnlyImgDir, sImagePrefix, sInOnlyImagePrefix, bDoNotAllowLossyCompression, iMinSwfVersion))
					{
						bSuccess = true;
					}
				}

				if (!bSuccess)
				{
					goto error;
				}
			}
			catch (Exception e)
			{
				Console.Error.WriteLine("Cooking failed: " + e.Message);
				Console.Error.Write(e.StackTrace);
				goto error;
			}

			return 0;

		error:
			Console.Error.Write("Invalid Arguments: ");
			for (int i = 0; i < asArgs.Length; ++i)
			{
				if (i != 0)
				{
					Console.Error.Write(" ");
				}
				Console.Error.Write(asArgs[i]);
			}
			Console.Error.WriteLine("Usage: FalconCooker.exe <input_swf_filename> -o <output_fcn_filename> -img_dir <images_folder>");
			return 1;
		}
	}
} // namespace FalconCooker
