//
//  ValidationTableViewController.swift
//  HPControlledPrintConsumer
//
//  Created by Fredy on 9/18/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

import UIKit

class ValidationTableViewController: UITableViewController {    
    @IBOutlet weak var uriTokenTextField: UITextField!
    @IBOutlet weak var validationOnOffSwitch: UISwitch!
    
    var setupTableViewController: SetupTableViewController?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        validationOnOffSwitch.addTarget(self, action: Selector("switchStateChanged:"), forControlEvents: UIControlEvents.ValueChanged)

        if let doValidation = setupTableViewController?.useToken {
            validationOnOffSwitch.setOn(doValidation, animated: true)
            if let source = setupTableViewController?.printSource {
                uriTokenTextField.text = source
            }
        }
        
        // Uncomment the following line to preserve selection between presentations
        // self.clearsSelectionOnViewWillAppear = false

        // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
        // self.navigationItem.rightBarButtonItem = self.editButtonItem()
    }

    func switchStateChanged(switchState: UISwitch) {
        if switchState.on {
            uriTokenTextField.text = "QPLESLIKETOKEN-2321321312321321321"
        } else {
            uriTokenTextField.text = "http://www.pdfpdf.com/samples/Sample5.PDF"
        }
    }
    
    override func viewWillDisappear(animated: Bool) {
        setupTableViewController?.printSource = uriTokenTextField.text!
        setupTableViewController?.useToken = validationOnOffSwitch.on
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    // MARK: - Table view data source
/*
    override func numberOfSectionsInTableView(tableView: UITableView) -> Int {
        // #warning Potentially incomplete method implementation.
        // Return the number of sections.
        return 0
    }

    override func tableView(tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        // #warning Incomplete method implementation.
        // Return the number of rows in the section.
        return 0
    }*/

    /*
    override func tableView(tableView: UITableView, cellForRowAtIndexPath indexPath: NSIndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCellWithIdentifier("reuseIdentifier", forIndexPath: indexPath) as! UITableViewCell

        // Configure the cell...

        return cell
    }
    */

    /*
    // Override to support conditional editing of the table view.
    override func tableView(tableView: UITableView, canEditRowAtIndexPath indexPath: NSIndexPath) -> Bool {
        // Return NO if you do not want the specified item to be editable.
        return true
    }
    */

    /*
    // Override to support editing the table view.
    override func tableView(tableView: UITableView, commitEditingStyle editingStyle: UITableViewCellEditingStyle, forRowAtIndexPath indexPath: NSIndexPath) {
        if editingStyle == .Delete {
            // Delete the row from the data source
            tableView.deleteRowsAtIndexPaths([indexPath], withRowAnimation: .Fade)
        } else if editingStyle == .Insert {
            // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
        }    
    }
    */

    /*
    // Override to support rearranging the table view.
    override func tableView(tableView: UITableView, moveRowAtIndexPath fromIndexPath: NSIndexPath, toIndexPath: NSIndexPath) {

    }
    */

    /*
    // Override to support conditional rearranging of the table view.
    override func tableView(tableView: UITableView, canMoveRowAtIndexPath indexPath: NSIndexPath) -> Bool {
        // Return NO if you do not want the item to be re-orderable.
        return true
    }
    */

    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject?) {
        // Get the new view controller using [segue destinationViewController].
        // Pass the selected object to the new view controller.
    }
    */

}
