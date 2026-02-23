# GetFolderStructure
## Purpose
Allows developers to quickly get their folder and file structure as json.

## Usage
Note: It is recommended to add getFolderStructure.exe to your PATH for easy access.

Usage: `cmd /k getFolderStructure.exe <directory> <max_depth>`

## Results
Outputs a json into the console, containing the folder and file structure of the given directory.
`<max_depth>` is the folder depth to check. A depth of 0 only reveals files directly within the directory. e.g.
```json
{
  ".git": ,
  "include": ,
  "src":
}
```
A `<max_depth>` of 1 will reveal those folders, and additionally what is inside them. e.g.
```json
{
  ".git": {
    "config": null,
        ...
  },
  "include": {
    "getFolderStructure.cpp": null,
        ...
  },
  "src": {
    "main.cpp": null,
        ...
  }
}
// And so on...
```
