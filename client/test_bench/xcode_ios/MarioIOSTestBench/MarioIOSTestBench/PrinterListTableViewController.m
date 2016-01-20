//
//  PrinterListTableViewController.m
//  MarioIOSTestBench
//
//  Created by Brian C Sparks on 3/16/15.
//  Copyright (c) 2015 HP, inc. All rights reserved.
//

#import "PrinterListTableViewController.h"
#import "AppDelegate.h"
#import "Printer.h"
#include "hp_sap.h"

@interface PrinterListTableViewController ()

- (int)dispatch:(char const *)message identifiedBy:(int)identifier forTransaction:(int)transaction_id withP1:(uint8 const *)p1 andParams:(sap_params const *)params;
- (int)onPrinterIp:(char const *)ip forKey:(char const*)key andValue:(char const*)value;

- (int)onPrinterData:(Printer *)printer forKey:(char const*)key;

// Printer List has changed message
- (int)onPrinterListChanged;
- (int)onBeginNewPrinterList;
- (int)sendPrinterListChangedMessage;

// Update the UI
- (void) reloadData:(NSMutableArray *)printersX;

// The displayed printer list, and the index list
@property NSMutableArray *printers;
@property NSMutableDictionary *printerIndexes;

// The printer list and index list as it comes in from MWP
@property NSMutableArray *buildoutPrinters;
@property NSMutableDictionary *buildoutPrinterIndexes;

- (void) dealloc;

@end

// -------------------- MWP dispatchers --------------------
int sap_printer_list_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, sap_params const * params)
{
  PrinterListTableViewController * controller = (__bridge PrinterListTableViewController *)(app_data);
  
  return [controller dispatch:msg identifiedBy:ident forTransaction:transaction_id withP1:p1 andParams:params];
}
// -------------------- MWP dispatchers (end) --------------------

@implementation PrinterListTableViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  
  // Uncomment the following line to preserve selection between presentations.
  // self.clearsSelectionOnViewWillAppear = NO;
  
  // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
  // self.navigationItem.rightBarButtonItem = self.editButtonItem;
  
  [self onBeginNewPrinterList];

  hp_sap_register_handler("printer_list", (__bridge void *)(self), sap_printer_list_callback);
  
  hp_sap_send_full_printer_list();
}

- (void)dealloc {
  hp_sap_deregister_handler("printer_list");
}

-(int) dispatch:(char const *)message identifiedBy:(int)identifier forTransaction:(int)transaction_id withP1:(uint8 const *)p1 andParams:(sap_params const *)params {

  char const * ip = (char const *)p1;
  char const * p2 = params ? params->p2 ? (char const *)params->p2 : "" : "";
  char const * p3 = params ? params->p3 ? (char const *)params->p3 : "" : "";
  
  //printf("PrinterList handling %s\n", message);
  
  // A new (full) printer list
  if (strcmp(message, HP_SAP_BEGIN_NEW_PRINTER_LIST_MSG) == 0) {
    [self onBeginNewPrinterList];
    
  // A single printer attribute changed
  } else if (strcmp(message, HP_SAP_PRINTER_ATTRIBUTE_MSG) == 0) {
    //printf("printer attribute!!: {%s}: {%s}:{%s}\n", ip, p2, p3);
    [self onPrinterIp:ip forKey:p2 andValue:p3];
    
  // A single printer attribute was removed
  } else if (strcmp(message, HP_SAP_RM_PRINTER_ATTRIBUTE_MSG) == 0) {
    // TODO
    printf("Remove attribute: {%s}: %s, \n", p1 ? (char const *)p1 : "", p2);
    
  // MWP is done enumerating printer attributes
  } else if (strcmp(message, HP_SAP_END_PRINTER_ENUM_MSG) == 0) {
    return [self onPrinterListChanged];
    
  }
  
  return 1;
}

- (int)onBeginNewPrinterList {

  _printers = [[NSMutableArray alloc] init];
  _printerIndexes = [[NSMutableDictionary alloc] init];
  _buildoutPrinters = [[NSMutableArray alloc] init];
  _buildoutPrinterIndexes = [[NSMutableDictionary alloc] init];

  return 1;
}

- (int)onPrinterIp:(char const *)ip_ forKey:(char const*)key_ andValue:(char const*)value_ {
  
  NSString * ip    = [NSString stringWithUTF8String:ip_];
  NSString * key   = [NSString stringWithUTF8String:key_];
  NSString * value = [NSString stringWithUTF8String:value_];
  
  NSNumber * index_ = [_buildoutPrinterIndexes valueForKey:ip];
  int index = [[_buildoutPrinterIndexes valueForKey:ip] intValue];
  
  Printer* printer;
  if (index_ != nil) {
    printer = [_buildoutPrinters objectAtIndex:index];
  } else {
    
    // Create a new Printer object
    printer = [[Printer alloc] init];
    printer.ipAddress = ip;
    [_buildoutPrinters addObject:printer];
    
    [_buildoutPrinterIndexes setValue:[NSNumber numberWithUnsignedLong:[_buildoutPrinters count] - 1] forKey:ip];
  }
  
  [printer setKey:key andValue:value];
  [self sendPrinterListChangedMessage];
  
  return 1;
}

- (int)onPrinterData:(Printer*)printer forKey:(char const*)key{
  
  return 1;
}

-(int) sendPrinterListChangedMessage {
  return 0;
}

-(int) onPrinterListChanged {
  
  // We are safe from MWP sending printer attributes while processing this message
  _printers       = _buildoutPrinters;
  _printerIndexes = _buildoutPrinterIndexes;

  [self performSelectorOnMainThread:@selector(reloadData:) withObject:_printers waitUntilDone:YES];
  return 1;
}

-(void) reloadData:(NSMutableArray *)printersX {

  [self.tableView reloadData];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // Return the number of rows in the section.
  return [_printers count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ListPrototypeCell" forIndexPath:indexPath];
    
    // Configure the cell...
  Printer * printer = [_printers objectAtIndex:indexPath.row];
  cell.textLabel.text = [NSString stringWithFormat:@"%@ [%@]", printer.displayName, printer.ipAddress];

  return cell;
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

#pragma mark - Table view delegate
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  
//  [tableView deselectRowAtIndexPath:indexPath animated:NO];
  
  AppDelegate * appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
  
  Printer *printer = [self.printers objectAtIndex:indexPath.row];
  printf("User chose %s %s\n", [printer.ipAddress UTF8String], [printer.displayName UTF8String]);
  
  [appDelegate printerChosen:printer.ipAddress];
  
  
//  [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
  
}

@end
