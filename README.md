# mindthemap

A Plan 9/9front-style mind mapping tool with vim-like controls.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Features

- Beautiful Rio-inspired visual design with rounded corners and gentle colors
- Vim-like modal interface (Normal and Insert modes)
- Intuitive keyboard controls:
  - `i` - Enter insert mode
  - `Esc` or `Enter` - Return to normal mode
  - `Tab` - Add child node
  - `Enter` - Add sibling node
  - `d` - Delete current node
  - Vim navigation: `h` (parent), `j` (next sibling), `k` (prev sibling), `l` (first child)
- Mouse interaction:
  - Click and drag any node (including root) to manually position it
  - Nodes snap to grid for clean alignment
- File operations (in normal mode):
  - `w` - Write mind map to file
  - `r` - Read mind map from file
  - `<` - Read from command
  - `>` - Write to command
  - `q` - Quit
- Visual features:
  - Alternating node colors by depth
  - Selected node highlighting
  - Special root node styling
  - Curved connection lines with dots
  - Auto-sizing nodes based on content
  - Status bar showing mode and version

## Usage

```
mindthemap [file]
```

If a file is specified, it will be loaded on startup. Otherwise, a new mind map will be created with a "Main Topic" root node.

## Building

Requires Plan 9/9front development environment. Build using mk:

```
mk
```

## File Format

Mind maps are saved in a simple text format:
```
NODE text x y manual_pos nchildren
```
where:
- `text` is the node content
- `x,y` are the node coordinates
- `manual_pos` indicates if the node was manually positioned
- `nchildren` is the number of child nodes that follow

## Version

Current version: 0.1

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

