import boto3

def lambda_handler(event, context):
    dynamodb = boto3.resource('dynamodb')
    table = dynamodb.Table('dustbinTable')
    
    response = table.scan()
    
    items = response['Items']
    sorted_items = sorted(items, key=lambda x: x['timestamp'], reverse=True)
    distance = event.get('Distance')
    
    
    if distance is not None:
        # If the distance is less than 3 cm, send email notification
        if distance < 3:
            sns_client = boto3.client('sns', region_name='eu-west-3')
            sns_topic_arn = 'arn:aws:sns:eu-west-3:369012686875:distance'  # Update to the new SNS topic ARN
                
            # Email parameters
            subject = "Warning: !!!! Container is Full !!!!"
            message = f"The Bin is full, and please stop loading into the Bin. Empty it. Percentage filled: {(100 - distance) / 100 * 100:.2f}%"

            # Send email notification
            response = sns_client.publish(
                TopicArn=sns_topic_arn,
                Subject=subject,
                Message=message
            )

            print("Email notification sent successfully:", response)

    
    return sorted_items
    
    