
import UIKit

class PrintViewController: UIViewController, ControlledPrintDelegate {
    
    @IBOutlet weak var printJobStatusLabel: UILabel!
    @IBOutlet weak var printerIpLabel: UILabel!
    @IBOutlet weak var printerNameLabel: UILabel!
    
    var spinner = UIActivityIndicatorView(activityIndicatorStyle: UIActivityIndicatorViewStyle.Gray)
    var printJob: PrintJob?
    var cpl: ControlledPrint?
    
    override func viewDidLoad() {
        self.cpl = ControlledPrint(controlledPrintDelegate: self)
        
        dispatch_async(dispatch_get_main_queue()) {
            self.printerNameLabel.text = self.printJob!.printerName
            self.printerIpLabel.text = self.printJob!.printerIp
            self.printJobStatusLabel.text = "Please wait ..."
        }        
        showSpinner()
        if (self.cpl?.print(printJob!.printSource, printerIp: printJob!.printerIp) == false) {
            //Proccess error
        }
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }
    
    func showSpinner() {
        self.view.addSubview(self.spinner)
        self.spinner.center = self.view.center
        self.spinner.frame = CGRectMake((self.view.frame.size.width/2) - 10, (self.view.frame.size.height/2), 20, 20);
        self.spinner.startAnimating()
    }
    
    // Mark: - ControlledPrintDelegate implementation
    func printJobStatus(status: String) {
        dispatch_async(dispatch_get_main_queue()) {
            self.printJobStatusLabel.text = status
        }
        println(status)
        if status == "Done" {
            dispatch_async(dispatch_get_main_queue()) {
                self.spinner.removeFromSuperview()
            }
        }
    }
    
    func printersFound(printers: Dictionary<String, HPPrinterAttributes>) {
    }
    
}
