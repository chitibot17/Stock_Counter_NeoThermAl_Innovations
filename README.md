# Stock_Counter_NeoThermAl_Innovations
A smart stock management system designed using ESP8266 and strain gauge load cell.
We are developing an integrated system that can detect industrial parts of various sizes and automate their counting. 

**PROJECT DESCRIPTION**
![image](https://user-images.githubusercontent.com/100292400/204848784-c56567b3-9ec2-409d-83b1-338fcadf99c5.png)

**Introduction:**

When I first started working on an industrial product, I realized that even small and seemingly simple products had tens of individual parts of all shapes, sizes, materials and colors. It was extremely difficult to keep a track of these parts, their stock and daily assembly/sub-assembly counts. For the sole purpose of inventory management and stock purchase, a lot of companies have expensive systems or teams in place. However, for smaller companies that are just scaling up, it becomes a nightmare, especially when each member is already doing multiple jobs. Hence, a lot of times a part goes out of stock due to negligence, resulting in total disruption of sales and operations for some time.

I wanted to develop something that would be affordable and could automate the entire process from start to end. It was an extremely difficult task, since as I mentioned earlier - the parts have vastly different properties. There are no ready-made solutions in the market that are fully automated and hence it was difficult to develop a method that would require negligible human intervention or data entry.

**Solution:**

The best way to count parts in bulk that are less than 1cm3 in total volume is to take their total weight and divide it by the individual part weight to get the part count. To get the initial part weight, we could weigh a set amount of each part on the scale and divide it by the known part count to get the accurate single part weight.
We also require a database containing other parameters for each part, like the cycle time, batch size, part name, etc. This database will be updated with the part weight and part count on daily basis to provide the stock count and daily stock usage can be calculated by subtracting each day's stock from that of the previous day.
 

###The load cell control device:

Our load cell control device basically uses the HX711 to amplify the load cell signals and uses a menu to navigate through the various functionalities. The following features are included:

*Normal weight display and measurement.
*Tare or zero scale.
*Add a part by entering its UID and part weight and find the batch part count accordingly.
*Remote WiFi setup.
*Onboard data storage on EEPROM.
*Data uploading on AWS-IoT service.
*Live dashboard for seeing stock quantities.
*Battery operated.
*Completely portable and modular, so that differently sensitive and capable load cell systems can be attached to the same control device as required.
