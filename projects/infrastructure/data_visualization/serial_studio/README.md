# Serial Studio Data Visualization Project
## Author: Daniel Park

### About

This is my venture in trying to use Serial Studio for Data Visualizing the CAN data. However, we decided to drop this in favor for Grafana due to lack of support for wireless connections (this is a great resource for physical connections through serial.)

### Instructions for Use

1. After Downloading Serial Studio, open the program through its app image from the downloading instructions.
 
2. Under Setup, select 'Parse via JSON project file' and select the 'testme.json' file.

3. Connect an Arduino and run the 'oem\_dummy\_test' Arduino file.

4. Select the port to whatever the Arduino is connected to (usually /ACMO) and select a baud rate of 19200 (or to whatever the Serial baud rate is at in the .ino file)

5. Select 'Connect' and Serial Studio should pick up the test file to show visualizations on Serial Studio
  
### Resources

Download Serial Studio by Following Instructions [Here](https://github.com/Serial-Studio/Serial-Studio/releases/tag/v1.1.7)

Serial Studio [Source Code](https://github.com/Serial-Studio/Serial-Studio/tree/v1.1.7)
