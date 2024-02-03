# pefti
Playlist and EPG Filter/Transformer for IPTV

pefti is a command-line application that takes one or more IPTV playlist files in M3U format which contain a list of IPTV channels, filters and transforms the channels according to the user configuration, then outputs a new playlist file. Features include renaming channels, regrouping channels, choosing the order of channels and groups, removing/adding/changing channel tags, and more.

In a future version, pefti will also be able to filter an Electronic Program Guide (EPG) file to match the new playlist file and reduce the file size by removing unwanted channel data.

Note that pefti is not interactive, it is intended to be run periodically to create an updated playlist with minimal effort.

## Build

To build pefti from source code:
```
git clone https://github.com/junglerock99/pefti.git
cd pefti
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
The pefti executable will be in the build directory.

A Windows executable can be downloaded from the Releases section.

## Usage

pefti runs from the command-line. Before running pefti, a configuration file must be created. The configuration file must be named `pefti.toml` and be stored in the same directory as the pefti executable. TOML format is used for the configuration, the full TOML specification is at https://toml.io, but it may be easiest to copy one of the configuration examples to get started.

## Example Configurations

Note that these examples show a small number of channels for brevity, a real playlist typically contains many channels.

### Merge Playlists

This configuration merges multiple playlist files into one new file. This would produce a new file named `new.m3u` which contains all of the IPTV channels from both iptv-1.m3u and iptv-2.m3u. This is a special case, because no groups or channels are blocked, pefti copies all groups and channels to the new.m3u output file. No filtering is performed.
```
[files]
input_playlists = ["https://example.com/iptv-1.m3u", "https://example.com/iptv-2.m3u"]
new_playlist = "new.m3u"
```

### Block Groups

This configuration filters groups from a playlist. The new file contains all channels from the input playlists except for channels that have a `group-title="News"` tag or a `group-title="Weather"` tag.
```
[files]
input_playlists = ["https://example.com/iptv-1.m3u", "https://example.com/iptv-2.m3u"]
new_playlist = "new.m3u"
[groups]
block = ["News","Weather"]
```

### Allow Groups

This configuration filters all groups from a playlist except for the allowed groups. The new file only contains channels from the input playlists that have a `group-title="News"` tag or a `group-title="Weather"` tag.
```
[files]
input_playlists = ["https://example.com/iptv-1.m3u", "https://example.com/iptv-2.m3u"]
new_playlist = "new.m3u"
[groups]
allow = ["News","Weather]
```

### Allow Channels

This configuration creates a new playlist that contains only the specified channels. The new playlist will contain only three channels, two in the USA group and one in the Canada group.
```
[files]
input_playlists = ["https://example.com/iptv-1.m3u", "https://example.com/iptv-2.m3u"]
new_playlist = "new.m3u"
[channels]
allow = [  
  "USA", [ 
  {allow = ["Nasa TV"], block = ["Media","UHD"], tags = {tvg-id = "Nasa TV HD"}},
  {allow = ["Reuters TV"]},
  ],
  "Canada", [ 
  {allow = ["TSC"], rename = "Shopping Channel"},
  ]
]
```

### All Currently Supported Features

The new playlist will contain two channels in the USA group, one channel in the Canada group, plus all channels in the Music group, in that order. If the URL for any channel matches one of the blocked URLs then that channel will be filtered out. If there are multiple instances of the same channel in the input playlists, then up to three instances will appear in the new playlist file because `number_of_duplicates` is set to 2. `duplicates_location` specifies where the duplicates will be located, in this case they will be placed at the end of the group. The qualities array specifies which instance of a channel is preferred when there are multiple instances. pefti will search the channel's name to find the preferred instance. `tags_block` allows you to filter tags, in this case the `tvg-logo` and `tvg-name` tags will not appear in the new playlist file for any channel. Any channel that contains "4K","SD" or "spanish" in the channel name in the input playlist file will be filtered out.
```
[files]
input_playlists = ["https://example.com/iptv-1.m3u", "https://example.com/iptv-2.m3u"]
new_playlist = "new.m3u"
[groups]
block = []
allow = ["Music"]
[urls]
block = ["https://example.com/channel/123", "https://example.com/channel/234"]
[channels]
number_of_duplicates = 2
duplicates_location = "append_to_group"
qualities = ["FHD","1080","HD","720"]
tags_block = ["tvg-logo","tvg-name"]
block = ["4K","SD","spanish"]
allow = [  
  "USA", [ 
  {allow = ["Nasa TV"], block = ["Media","UHD"], tags = {tvg-id = "Nasa TV HD"}},
  {allow = ["Reuters TV"]},
  ],
  "Canada", [ 
  {allow = ["TSC"], rename = "Shopping Channel"},
  ]
]
```

## Configuration Options

The section describes the configuration options used by pefti.

The configuration options are grouped using TOML tables. A table starts with the table name in square brackets, e.g. `[groups]`. Following the table name are the options for that table, written as key=value pairs. More than one value can be assigned by placing the values in a TOML array using square brackets, e.g. `option = ["value1","value2","value3"]`. 

### [files] table
Key | Type | Value 
--- | --- | ---
input_playlists | Array of text strings | URLs of the input playlists
new_playlist | Text string | Filename for the new playlist

### [groups] table
Key | Type | Value 
--- | --- | ---
block | Array of text strings | Names of groups to block. A channel will be blocked if it has a `group-title` tag that matches any of the text strings in this array. Case sensitive.
allow | Array of text strings | Names of groups to allow. All channels with a `group-title` tag that matches any of the text strings in this array will be copied to the new playlist file. Case sensitive.

### [urls] table
Key | Type | Value 
--- | --- | ---
block | Array of text strings | URLs to filter out. A channel will be blocked if its URL matches any of the text strings in this array.

### [channels] table
Key | Type | Value 
--- | --- | ---
number_of_duplicates | Integer | When there are multiple instances of a channel in the input playlist files, specifies how many duplicate instances to copy to the new playlist file.
duplicates_location | Text string | When there are multiple instances of a channel and `number_of_duplicates` is greater than zero, specifies where the duplicate channels are to be located in the new playlist file. Options are `inline` where all instances are located together, or `append_to_group` where duplicate instances are moved to the end of the group.
qualities | Array of text strings | When there are multiple instances of a channel, searches the channel names to find the best quality. This array should contain the text strings to search for in order of preference, e.g. `["4K","1080","720"]`.
tags_block | Array of text strings | Tags to block, e.g. `["tvg-id","tvg-name"]`.
block | Array of text strings | Blocks a channel if its name contains any of these text strings
allow | Array | Contains repeating group-name, allowed channels, group-name, allowed channels etc. Each allowed channel is a table containing text strings that the channel name must contain, text strings that the channel cannot contain and tags to add to the channel. See example above.

## Source Code

pefti is written in C++20 and follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) except that it uses exceptions.

## Performance

On a PC with a 3.2GHz i5-4460 CPU and a solid-state drive, it takes 570 milliseconds to process two m3u files containing a total of 10,964 channels and create a new playlist containing 90 channels. 

## Technical

pefti only recognizes two extended M3U directives: #EXTM3U and #EXTINF.

## To Do

* Add command-line option for location of config file
* Allow some configuration options to be specified on the command-line
* Add option to use the original channel name
* Add option to add the original channel name to the new playlist as a comment for debugging
* Add test cases
