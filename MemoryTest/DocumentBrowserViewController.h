//
//  DocumentBrowserViewController.h
//  MemoryTest
//
//  Created by Ryan Macnak on 5/24/23.
//

#import <UIKit/UIKit.h>

@interface DocumentBrowserViewController : UIDocumentBrowserViewController

- (void)presentDocumentAtURL:(NSURL *)documentURL;

@end
