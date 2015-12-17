
import Foundation

protocol ControlledPrintDelegate {
    func printersFound(_: Dictionary<String, HPPrinterAttributes>)
    func printJobStatus(_: String)
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
    
    func initialize(stack: ServerStack, completion: ((InitStatus) -> ())) {
        ControlledPrint.cpl.initialize(stack, withCompletion:{(status: InitStatus) in
            completion(status)
        })
        var analyticsModel = GoogleAnalyticsModel();
        analyticsModel.eventCategory = "ConsumerAppControlledPrint";
        analyticsModel.eventAction = "ConsumerAppInitialize";
        postGoogleMetrics("event", analyticsMdl: analyticsModel)
    }
    
    func validateToken(token: String, completion: ((Bool) -> ())) {
        ControlledPrint.cpl.validateToken(token, withCompletion: {(valid: Bool) in
            completion(valid)
        })
    }
    
    func printerListUpdateInterval(inSeconds: Int32) {
        ControlledPrint.cpl.setScanIntervalSeconds(inSeconds)
    }
    
    func scanForPrinters() -> Bool {
        
        var analyticsModel = GoogleAnalyticsModel();
        analyticsModel.eventCategory = "ConsumerAppControlledPrint";
        analyticsModel.eventAction = "ConsumerAppScanForPrinters";
        postGoogleMetrics("event", analyticsMdl: analyticsModel)
        return ControlledPrint.cpl.scanForPrinters()
    }
    
    func print(source: String, printerIp:String) -> Bool{
        var printer = HPPrinter()
        printer.ipAddress = printerIp
        
        var printJob = HPPrintJobRequest()
        printJob.tokenId = source
        printJob.providerId = ProviderQples
        
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
        self.foundPrinters.removeAll(keepCapacity: true)
        for (ip, printer) in printers.printers {
            self.foundPrinters[ip as! String] = printer as? HPPrinterAttributes;
        }
        controlledPrintDelegate?.printersFound(self.foundPrinters);
    }
    
    // MARK: - Implement GoogleAnalyticsFunctions
    // DONE: Move variables to GAModel, and make them constant
    // Add functions to this file so screen views can call them
    // DONE: Enum change the name to be more descriptive
    
    func postGoogleMetrics(type: String, analyticsMdl: GoogleAnalyticsModel){
        switch type{
            case "event":
                NSLog("Logging Analytics Event from ControlledPrint.swift")
                ControlledPrint.cpl.postGoogleAnalyticsMetrics(GoogleAnalyticsTypeEnumEvent, withParams: analyticsMdl)

            case "screen":
                NSLog("Logging Analytics Screen from ControlledPrint.swift")
                ControlledPrint.cpl.postGoogleAnalyticsMetrics(GoogleAnalyticsTypeEnumScreen, withParams: analyticsMdl);
            case "crash":
                // Crashes are uploaded automatically
                NSLog("Logging Analytics Crash from ControlledPrint.swift")
            default:
                NSLog("Default")
        }
    }
}