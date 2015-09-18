

import UIKit

class SetupViewController: UIViewController, UIPickerViewDataSource, UIPickerViewDelegate {
    @IBOutlet weak var stackPickerView: UIPickerView!
    @IBOutlet weak var urlTextField: UITextField!    
    
    override func viewDidLoad() {
        super.viewDidLoad()
        stackPickerView.delegate = self
        stackPickerView.dataSource = self
        stackPickerView.showsSelectionIndicator = true

        // Do any additional setup after loading the view.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    // MARK: - UIPickerViewDataSource
    
    func numberOfComponentsInPickerView(pickerView: UIPickerView) -> Int {
        return 1
    }
    
    func pickerView(pickerView: UIPickerView, numberOfRowsInComponent component: Int) -> Int {
        return 3
    }
    
    // MARK - UIPicerViewDelegate
    
    func pickerView(pickerView: UIPickerView, didSelectRow row: Int, inComponent component: Int) {
        //row selected
    }
    
    func pickerView(pickerView: UIPickerView, titleForRow row: Int, forComponent component: Int) -> String! {
        switch row {
        case 0 :
            return "Development"
        case 1 :
            return "External Test"
        case 2 :
            return "Production"
        default :
            return "Development"
        }
    }
    
    func pickerView(pickerView: UIPickerView, rowHeightForComponent component: Int) -> CGFloat {
        return 50.0
    }
    
    func pickerView(pickerView: UIPickerView, widthForComponent component: Int) -> CGFloat {
        return 200.0
    }
    
    // MARK: - Navigation
    
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject?) {
        var printerListView = segue.destinationViewController as! PrinterListViewController
        
        var selectedStack = ServerStackDevelopment
        if let selectedRow = self.stackPickerView?.selectedRowInComponent(0) {
            switch selectedRow {
            case 0 :
                selectedStack = ServerStackDevelopment
            case 1 :
                selectedStack = ServerStackExternalTest
            case 2 :
                selectedStack = ServerStackProduction
            default :
                selectedStack = ServerStackDevelopment
            }
        }
        printerListView.selectedServerStack = selectedStack
        printerListView.imageUrl = urlTextField.text!
    }
}