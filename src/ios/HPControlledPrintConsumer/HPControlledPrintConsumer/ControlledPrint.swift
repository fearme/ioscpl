
import Foundation

protocol ControlledPrintDelegate {
    func printersFound(Dictionary<String, HPPrinterAttributes>)
    func printJobStatus(String)
}

class ControlledPrint : NSObject, HPPrinterAttributesDelegate {
    
    //Ensure the use of only one instance of HPControlledPrintManager
    private static var cpl = HPControlledPrintManager()
    
    private var foundPrinters = Dictionary<String, HPPrinterAttributes>()
    private let controlledPrintDelegate: ControlledPrintDelegate?
    
    
    init(controlledPrintDelegate: ControlledPrintDelegate) {
        self.controlledPrintDelegate = controlledPrintDelegate
        super.init()
        ControlledPrint.cpl.printerAttributesDelegate = self
    }
    
    func initialize(stack: ServerStack, token: String, validation: Bool, completion: ((InitStatus) -> ())) {
        ControlledPrint.cpl.initialize(stack, withToken: token, doValidation: validation, withCompletion:{(status: InitStatus) in
            completion(status)
        })
    }
    
    func printerListUpdateInterval(inSeconds: Int32) {
        ControlledPrint.cpl.setScanIntervalSeconds(inSeconds)
    }
    
    func scanForPrinters() -> Bool {
        return ControlledPrint.cpl.scanForPrinters()
    }
    
    func print(source: String, printerIp:String) -> Bool{
        var printer = HPPrinter()
        printer.ipAddress = printerIp
        
        var printJob = HPPrintJobRequest()
        printJob.tokenId = source
        printJob.providerId = ProviderQples
        printJob.hardwareId = "FDC_PhoneID"
        
        return ControlledPrint.cpl.print(printer, withJobRequest: printJob)
    }
    
    func exit() {
        ControlledPrint.cpl.exit()
    }
    
    func setProxy(host: String, port: String) {
        ControlledPrint.cpl.proxy(host, onPort: port)
    }
    
    // MARK: - Implement PrinterAttributesDelegate
    
    func didReceivePrintJobStatus(status: String) {
        controlledPrintDelegate?.printJobStatus(status);
    }
    
    func didReceivePrinters(printers: HPDiscoveredPrinters) {
        for (ip, printer) in printers.printers {
            self.foundPrinters[ip as! String] = printer as? HPPrinterAttributes;
        }
        controlledPrintDelegate?.printersFound(self.foundPrinters);
    }

}

