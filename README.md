# OpenGL-Text-Editor

## Goals
- learning about data structures suited for text editors such as Rope, Gap Buffer, Piece Table
    - I decided on implementing rope data structures
- Learn about efficient way of implementing undo & redo commands
- Learn about rendring in Opengl
- Learn about new features of C23 

###  Requirements
- OpenGL
- XLib
- [Runara](https://github.com/cococry/runara) 

### TO-DO

- Redo cursor implementation
    - currently cursor contains info about location in line and pointer to current line
    - better implementation would be to have LineIndex that contains offset of line in buffer, 
        the size of line, and number of lines, than cursor would only contain indexes
- Add message for user input when saving and loading file
- Undo & Redo
    - Fix cursor alligment after undo and redo, currently cursor go back to start of text
    - memmory cleanup of previous state
- Add word wrapping
- Seperate render function in editor.c
- Code cleanup
- maybe add simple GUI (maybe raigui or imgui) 
