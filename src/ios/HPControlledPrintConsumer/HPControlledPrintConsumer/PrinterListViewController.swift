
import UIKit

class PrinterListViewController: UIViewController, UITableViewDataSource, UITableViewDelegate, ControlledPrintDelegate, UIAlertViewDelegate {

    @IBOutlet weak var availablePrintersTableView: UITableView!
    @IBOutlet weak var printJobStatusLabel: UILabel!
    
    static var printerScanStarted = false
    
    var spinner = UIActivityIndicatorView(activityIndicatorStyle: UIActivityIndicatorViewStyle.Gray)
    var cpl: ControlledPrint?
    var printers = [HPPrinterAttributes]()
    var printJob: PrintJob?
    var selectedServerStack: ServerStack = ServerStackDevelopment
    var printSource = "QPLESLIKETOKEN-2321321312321321321"
    var doValidation = true
    var clearPrinterList = false
    
    let availablePrinterSection = 0

    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.cpl = ControlledPrint(controlledPrintDelegate: self)
        self.cpl?.printerListUpdateInterval(3)
        self.cpl?.setProxy("proxy.atlanta.hp.com", port: "8080")
        //self.cpl?.setProxy("web-proxy", port: "8088")

        showSpinner()
        
        //Valid Token: QPLESLIKETOKEN-2321321312321321321
        //Valid File: http://www.pdfpdf.com/samples/Sample5.PDF
        self.cpl?.initialize(selectedServerStack, completion:{(status: InitStatus) -> () in
            if (status.value == InitStatusServerStackNotAvailable.value) {
                self.spinner.removeFromSuperview()
                self.clearPrinterList = true
                let alert = UIAlertView(title: "Server Error", message: "Unable to connect to the Server Stack.", delegate: self, cancelButtonTitle: "OK")
                alert.show()
                
            } else if (status.value == InitStatusServerStackAvailable.value) {
                if (self.doValidation) {
                    
                    if let delegate = UIApplication.sharedApplication().delegate as? AppDelegate {                        
                        if let token = delegate.launchToken {
                            self.printSource = token
                        }
                    }
                    
                    self.cpl?.validateToken(self.printSource, completion: {(valid: Bool) -> () in
                        if (!valid) {
                            self.spinner.removeFromSuperview()
                            self.clearPrinterList = true
                            let alert = UIAlertView(title: "Invalid Coupon", message: "Coupon Token is INVALID.", delegate: self, cancelButtonTitle: "OK")
                            alert.show()
                            
                        } else {
                            self.clearPrinterList = false
                            self.cpl!.scanForPrinters()
                        }
                    })
                    
                } else {
                    self.clearPrinterList = false
                    self.cpl!.scanForPrinters()
                }
            }
        })
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }

    func showSpinner() {
        self.view.addSubview(self.spinner)
        self.spinner.center = self.view.center
        self.spinner.frame = CGRectMake((self.view.frame.size.width/2) - 10, (self.view.frame.size.height/2) - 10, 20, 20);
        self.spinner.startAnimating()
    }
    
    // MARK: - ControlledPrintDelegate protocol - implementation
    
    func printersFound(printersDictionary: Dictionary<String, HPPrinterAttributes>) {
        //NSLog("Printer count: %d", printersDictionary.count)
        self.printers.removeAll(keepCapacity: true)
        for (_, printer) in printersDictionary {
            self.printers.append(printer)
        }
        
        var range = NSMakeRange(0, 1)
        var indexSet = NSIndexSet(indexesInRange: range)
        self.availablePrintersTableView.reloadSections(indexSet, withRowAnimation: .None)
        
        self.spinner.removeFromSuperview()
    }
    
    func printJobStatus(status: String) {
        dispatch_async(dispatch_get_main_queue()) {
            self.printJobStatusLabel?.text = status
        }
    }

    // MARK: - UITableViewDataSource protocol implementations
    
    func numberOfSectionsInTableView(tableView: UITableView) -> Int {
        return 1
    }
    
    func tableView(tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        var rows = 0
        if !clearPrinterList && section == availablePrinterSection {
            rows = self.printers.count
        }
        return rows
    }
    
    func tableView(tableView: UITableView, cellForRowAtIndexPath indexPath: NSIndexPath) -> UITableViewCell {
        var cell = tableView.dequeueReusableCellWithIdentifier("cell", forIndexPath: indexPath) as! UITableViewCell
        
        if indexPath.section == availablePrinterSection {
            let printerAttributes = self.printers[indexPath.row]
            
            cell.textLabel?.text = printerAttributes.name
            cell.detailTextLabel?.text = printerAttributes.ip
            
            if printerAttributes.isSupported {
                cell.imageView?.image = UIImage(named: "PrinterSupported")
            } else {
                cell.imageView?.image = UIImage(named: "PrinterNotSupported")
            }
        }
        
        return cell
    }
    
    func tableView(tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        var header = ""
        if section == availablePrinterSection {
            header = "Available Printers"
        }
        return header
    }
    
    func tableView(tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        var height = CGFloat(30)
        if section == self.availablePrinterSection {
            height = CGFloat(30)
        }
        return height
    }
    
    
    // MARK: - UITableViewDelegate implementation
    
    func tableView(tableView: UITableView, didSelectRowAtIndexPath indexPath: NSIndexPath) {
        let cell = tableView.cellForRowAtIndexPath(indexPath)
        if indexPath.section == availablePrinterSection {

        }
    }
    
    
    // MARK: - Segue
    
    override func shouldPerformSegueWithIdentifier(identifier: String?, sender: AnyObject?) -> Bool {
        var doSegue = true
        
        if let indexPath = self.availablePrintersTableView.indexPathForSelectedRow() {
            let printerAttributes = self.printers[indexPath.row]
            if !printerAttributes.isSupported {
                let alert = UIAlertView(title: "Unsupported Printer", message: "Sorry, this printer is not supported.", delegate: self, cancelButtonTitle: "OK")
                doSegue = false
                alert.show()
            }
        }
        return doSegue
    }
        
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject?) {
        var printScene = segue.destinationViewController as! PrintViewController
        
        if let indexPath = self.availablePrintersTableView.indexPathForSelectedRow() {
            if let cell = self.availablePrintersTableView.cellForRowAtIndexPath(indexPath) {
                let selectedPrinterName = cell.textLabel?.text
                let selectedPrinterIp = cell.detailTextLabel?.text
                self.printJob = PrintJob(printerName: selectedPrinterName!, printerIp: selectedPrinterIp!, printSource: self.printSource)
                printScene.printJob = self.printJob
            }
        }
    }
    
}

