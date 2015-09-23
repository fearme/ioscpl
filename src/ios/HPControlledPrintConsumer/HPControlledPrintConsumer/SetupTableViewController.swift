//
//  SetupTableViewController.swift
//  HPControlledPrintConsumer
//
//  Created by Fredy on 9/18/15.
//  Copyright (c) 2015 Secure Print & Mobile Enablement. All rights reserved.
//

import UIKit

class SetupTableViewController: UITableViewController {

    internal var selectedServerStack: ServerStack = ServerStackDevelopment
    internal var printSource: String = "QPLESLIKETOKEN-2321321312321321321"
    internal var useToken = true
    
    var serverStack: String?
    var uriToken: String?
    
    let tableSectionSetup = 0;
    let tableSectionButton = 1;
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

    override func tableView(tableView: UITableView, didSelectRowAtIndexPath indexPath: NSIndexPath) {
        let cell = tableView.cellForRowAtIndexPath(indexPath)
        
    }

    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject?) {
        // Get the new view controller using segue.destinationViewController.
        // Pass the selected object to the new view controller.
        
        if segue.identifier == "Validation" {
            var validationScene = segue.destinationViewController as! ValidationTableViewController
            validationScene.setupTableViewController = self
            
        } else if segue.identifier == "ServerStack" {
            var serverStackScene = segue.destinationViewController as! ServerStackViewController
            serverStackScene.setupTableViewController = self
            
        } else if segue.identifier == "Print" {
            var printerListView = segue.destinationViewController as! PrinterListViewController
            printerListView.selectedServerStack = self.selectedServerStack
            printerListView.printSource = self.printSource
            printerListView.doValidation = useToken
            
        }
    }

}
